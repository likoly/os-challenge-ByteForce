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

// Queue structure for client requests
typedef struct node
{
    request_t request;
    struct node *next;
} node_t;

node_t *head = NULL;
node_t *tail = NULL;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

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
// Function to enqueue a request based on priority
void enqueue(request_t request)
{
    pthread_mutex_lock(&queue_mutex);

    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->request = request;
    new_node->next = NULL;

    // If the queue is empty, add the new node as the head
    if (head == NULL)
    {
        head = tail = new_node;
    }
    else if (head->request.prio < request.prio)
    {
        // Insert at the beginning if it has the highest priority
        new_node->next = head;
        head = new_node;
    }
    else
    {
        // Insert in the middle or end based on priority
        node_t *current = head;
        while (current->next != NULL && current->next->request.prio >= request.prio)
        {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;

        // Update the tail if we inserted at the end
        if (new_node->next == NULL)
        {
            tail = new_node;
        }
    }

    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Function to dequeue a request (highest priority is always at the head)
request_t dequeue()
{
    pthread_mutex_lock(&queue_mutex);

    // Wait if the queue is empty
    while (head == NULL)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    node_t *highest_prio_node = head;
    request_t request = highest_prio_node->request;

    head = highest_prio_node->next;
    if (head == NULL)
    {
        tail = NULL;
    }

    free(highest_prio_node);
    pthread_mutex_unlock(&queue_mutex);

    return request;
}

// Brute-force search function for a range
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

// Worker thread function
void *worker(void *arg)
{
    while (1)
    {
        request_t request = dequeue();
        uint64_t key = 0;

        if (checkCache(request.hash, &key))
        {
            printf("Cache hit for hash\n");
        }
        else
        {
            key = bruteForceSearch(request.hash, request.start, request.end);
            if (key != 0)
            {
                addToCache(request.hash, key);
            }
        }

        key = htobe64(key);
        write(request.client_socket, &key, sizeof(key));
        close(request.client_socket);
    }
    return NULL;
}

// CTRL+C termination / interrupt handler
void terminationHandler(int sig)
{
    close(server_fd);
    exit(0);
}

// Function to parse client request
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

int main(int argc, char *argv[])
{
    signal(SIGINT, terminationHandler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

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

    listen(server_fd, 100);

    struct sockaddr_in cli_addr;
    int cli_length = sizeof(cli_addr);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    while (1)
    {
        int client_socket = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_length);
        if (client_socket < 0)
        {
            perror("Accept failed");
            exit(1);
        }

        char buffer[PACKET_REQUEST_SIZE];
        bzero(buffer, PACKET_REQUEST_SIZE);
        read(client_socket, buffer, PACKET_REQUEST_SIZE);

        request_t request = pack(buffer, client_socket);
        uint64_t key = 0;
        if (checkCache(request.hash, &key))
        {
            key = htobe64(key);
            write(request.client_socket, &key, sizeof(key));
            close(request.client_socket);
        }
        else
        {
            enqueue(request);
        }
    }

    return 0;
}
