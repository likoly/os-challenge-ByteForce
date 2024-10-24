#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>

#include "messages.h"

#define NUM_THREADS 12
#define CACHE_SIZE 1000 // Adjust this value based on expected cache usage

// Global socket variables
int server_fd;

// Cache structure for storing hashes and their corresponding keys
typedef struct
{
    uint8_t hash[32];
    uint64_t key;
} cache_entry_t;

cache_entry_t cache[CACHE_SIZE];
int cache_count = 0;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to hold client data
typedef struct
{
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t prio;
    int client_socket;
} request_t;

// Function to check the cache for a hash
int checkCache(uint8_t *hash, uint64_t *key)
{
    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < cache_count; i++)
    {
        if (memcmp(cache[i].hash, hash, 32) == 0)
        {
            *key = cache[i].key;
            pthread_mutex_unlock(&cache_mutex);
            return 1; // Hash found in the cache
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return 0; // Hash not found in the cache
}

// Function to add a result to the cache
void addToCache(uint8_t *hash, uint64_t key)
{
    pthread_mutex_lock(&cache_mutex);
    if (cache_count < CACHE_SIZE)
    {
        memcpy(cache[cache_count].hash, hash, 32);
        cache[cache_count].key = key;
        cache_count++;
    }
    pthread_mutex_unlock(&cache_mutex);
}

// Brute-force search function for a range with immediate result sending and canceling other threads
uint64_t bruteForceSearch(uint8_t *hash, uint64_t start, uint64_t end, int client_socket, pthread_t *threads, int num_threads)
{
    uint8_t calculatedHash[32];
    uint64_t key;
    for (uint64_t i = start; i < end; i++)
    {
        // Calculate hash
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, &i, 8);
        SHA256_Final(calculatedHash, &sha256);

        if (memcmp(hash, calculatedHash, 32) == 0)
        {
            key = i;

            // Immediately send the key to the client
            key = htobe64(key);
            write(client_socket, &key, 8);

            // Cancel all other threads
            for (int j = 0; j < num_threads; j++)
            {
                if (pthread_self() != threads[j]) // Don't cancel the current thread
                {
                    pthread_cancel(threads[j]);
                }
            }

            return key;
        }
    }
    return 0;
}

// Thread argument structure
typedef struct
{
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    int thread_id;
    uint64_t result;
    int client_socket;
    pthread_t *threads; // Pass the thread array to allow bruteForceSearch to cancel others
    int num_threads;
} thread_arg_t;

// Worker thread function
void *worker(void *arg)
{
    thread_arg_t *thread_arg = (thread_arg_t *)arg;

    // Set thread to be cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // Perform brute-force search and send result if found, canceling other threads
    thread_arg->result = bruteForceSearch(thread_arg->hash, thread_arg->start, thread_arg->end, thread_arg->client_socket, thread_arg->threads, thread_arg->num_threads);

    pthread_exit(NULL);
}

// CTRL+C termination / interrupt handler
void terminationHandler(int sig)
{
    close(server_fd);
    exit(0);
}

int main(int argc, char *argv[])
{
    // Initialize termination signal
    signal(SIGINT, terminationHandler);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Make sure the port is available
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Initialize socket structure and bind the socket to the port
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind failed");
        exit(1);
    }

    // Listen for client
    listen(server_fd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int cli_length = sizeof(cli_addr);

    // Accept client connections and enqueue requests
    while (1)
    {
        int client_socket = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_length);
        if (client_socket < 0)
        {
            perror("Accept failed");
            exit(1);
        }

        // Read in request through client socket
        char buffer[PACKET_REQUEST_SIZE];
        bzero(buffer, PACKET_REQUEST_SIZE);
        read(client_socket, buffer, PACKET_REQUEST_SIZE);

        // Extract request data
        request_t request;
        memcpy(request.hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
        memcpy(&request.start, buffer + PACKET_REQUEST_START_OFFSET, 8);
        memcpy(&request.end, buffer + PACKET_REQUEST_END_OFFSET, 8);
        request.start = be64toh(request.start);
        request.end = be64toh(request.end);
        request.client_socket = client_socket;

        uint64_t key = 0;
        // Check the cache before performing brute-force search
        if (checkCache(request.hash, &key))
        {
            // Key found in cache, no need to brute-force
            printf("Cache hit for hash\n");
            key = htobe64(key);
            write(request.client_socket, &key, 8);

            // Close the client socket
            close(request.client_socket);
        }
        else
        {
            // Split the range between NUM_THREADS threads
            pthread_t threads[NUM_THREADS];
            thread_arg_t thread_args[NUM_THREADS];
            uint64_t range = (request.end - request.start) / NUM_THREADS;

            for (int i = 0; i < NUM_THREADS; i++)
            {
                thread_args[i].start = request.start + i * range;
                thread_args[i].end = (i == NUM_THREADS - 1) ? request.end : thread_args[i].start + range;
                memcpy(thread_args[i].hash, request.hash, 32);
                thread_args[i].client_socket = client_socket; // Pass the client socket to each thread
                thread_args[i].threads = threads;             // Pass the thread array to allow cancellation
                thread_args[i].num_threads = NUM_THREADS;
                pthread_create(&threads[i], NULL, worker, &thread_args[i]);
            }

            // Wait for all threads to finish (canceled ones will stop immediately)
            for (int i = 0; i < NUM_THREADS; i++)
            {
                pthread_join(threads[i], NULL);
            }

            // Close the client socket
            close(request.client_socket);
        }
    }

    return 0;
}
