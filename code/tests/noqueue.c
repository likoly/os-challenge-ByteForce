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
#include <arpa/inet.h>

#include "messages.h"

#define NUM_THREADS 7
#define CACHE_SIZE 1000
#define QUEUE_SIZE 1000 // Array size for the priority queue
#define MAX_REQUESTS 500
// Mutexes and condition variables
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global socket variable
int server_fd;

// Cache structure for storing hashes and their corresponding keys
typedef struct
{
    uint8_t hash[32];
    uint64_t key;
} cache_entry_t;

cache_entry_t cache[CACHE_SIZE];
int cache_count = 0;

// Request structure for client data
typedef struct
{
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t prio;
    int client_socket;
} request_t;

// Queue for storing requests
request_t request_queue[QUEUE_SIZE];
int queue_size = -1;

// Function declarations
int checkCache(uint8_t *hash, uint64_t *key);
void addToCache(uint8_t *hash, uint64_t key);
uint64_t bruteForceSearch(uint8_t *hash, uint64_t start, uint64_t end);
request_t pack(char buffer[], int client_socket);

void *worker(void *arg);

int checkCache(uint8_t *hash, uint64_t *key)
{
    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < cache_count; i++)
    {
        if (memcmp(cache[i].hash, hash, 32) == 0)
        {
            *key = cache[i].key;
            pthread_mutex_unlock(&cache_mutex);
            printf("Cache hit for hash\n");
            return 1; // Hash found in the cache
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return 0; // Hash not found in the cache
}

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

uint64_t bruteForceSearch(uint8_t *hash, uint64_t start, uint64_t end)
{
    uint8_t calculatedHash[32];
    for (uint64_t i = start; i < end; i++)
    {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, &i, sizeof(i));
        SHA256_Final(calculatedHash, &sha256);
        if (memcmp(hash, calculatedHash, 32) == 0)
        {
            return i;
        }
    }
    return 0;
}

request_t pack(char buffer[], int client_socket)
{
    request_t request;
    memcpy(request.hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
    memcpy(&request.start, buffer + PACKET_REQUEST_START_OFFSET, 8);
    memcpy(&request.end, buffer + PACKET_REQUEST_END_OFFSET, 8);
    memcpy(&request.prio, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);
    request.start = be64toh(request.start);
    request.end = be64toh(request.end);
    request.client_socket = client_socket;
    return request;
}

// Worker thread function
void *worker(void *arg)
{
    request_t *request = (request_t *)arg;
    uint64_t key = 0;

    if (!checkCache(request->hash, &key))
    {
        key = bruteForceSearch(request->hash, request->start, request->end);
        if (key != 0)
        {
            addToCache(request->hash, key);
        }
    }

    key = htobe64(key);
    write(request->client_socket, &key, sizeof(key));
    close(request->client_socket);
    return NULL;
}

int main(int argc, char *argv[])
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(5000);

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 100);

    struct sockaddr_in cli_addr;
    int cli_length = sizeof(cli_addr);

    request_t requests[MAX_REQUESTS]; // Pre-allocated array for requests
    pthread_t threads[MAX_REQUESTS];  // Array for thread IDs
    int request_index = 0;

    while (1)
    {
        int client_socket = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_length);
        if (client_socket < 0)
        {
            perror("Accept failed");
            exit(1);
        }

        char buffer[1024]; // Adjust size as needed
        bzero(buffer, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));

        // Get the next request slot
        request_t *request = &requests[request_index];
        *request = pack(buffer, client_socket);

        uint64_t key = 0;

        if (checkCache(request->hash, &key))
        {
            key = htobe64(key);
            write(request->client_socket, &key, sizeof(key));
            close(request->client_socket);
        }
        else
        {
            if (pthread_create(&threads[request_index], NULL, worker, request) != 0)
            {
                perror("pthread_create failed");
                close(request->client_socket);
            }
            else
            {
                pthread_detach(threads[request_index]);             // Detach the thread to avoid needing to join it
                request_index = (request_index + 1) % MAX_REQUESTS; // Wrap around if we reach the limit
            }
        }
    }

    close(server_fd);
    return 0;
}