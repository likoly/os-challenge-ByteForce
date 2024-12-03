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

// LINKED QUEUE STARTS HERE
typedef struct node
{
    request_t request;
    struct node *next;
} node_t;

node_t *head = NULL;
node_t *tail = NULL;

node_t *newNode(request_t request)
{
    node_t *temp = (node_t *)malloc(sizeof(node_t));
    temp->request = request;
    temp->next = NULL;
    return temp;
}

void enqueue(request_t request)
{
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->request = request;
    new_node->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    if (head == NULL)
    {
        head = new_node;
    }
    else if (request.prio == 1)
    {
        // If prio = 1, skip to tail
        node_t *tail = head;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }
        tail->next = new_node;
    }
    else if (head->request.prio > request.prio)
    {
        // Insert at the head if it has higher priority
        new_node->next = head;
        head = new_node;
    }
    else
    {
        // Find the appropriate position based on priority
        node_t *current = head;
        while (current->next != NULL && current->next->request.prio <= request.prio)
        {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }

    // Signal only if the queue was previously empty
    if (new_node->next == NULL || head == new_node)
    {
        pthread_cond_signal(&queue_cond);
    }

    pthread_mutex_unlock(&queue_mutex);
}

request_t dequeue()
{
    pthread_mutex_lock(&queue_mutex);

    // Wait if the queue is empty
    while (head == NULL)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Peek at the request at the head
    node_t *temp = head;
    request_t request = temp->request;

    // Remove the head node
    head = head->next;
    if (head == NULL)
    {
        // If the queue becomes empty, reset the tail pointer
        tail = NULL;
    }

    free(temp);

    pthread_mutex_unlock(&queue_mutex);
    return request;
}
void *worker(void *arg)
{
    while (1)
    {
        request_t request = dequeue();
        uint64_t key = 0;

        if (!checkCache(request.hash, &key))
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

int main(int argc, char *argv[])
{
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
    serv_addr.sin_port = htons(5000);

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
