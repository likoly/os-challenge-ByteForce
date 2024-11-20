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
#include "funcs.h"

volatile uint64_t key;
request_t currentRequest;
pthread_t threads[NUM_THREADS];
uint64_t bruteForceSearch1(uint8_t *hash, uint64_t start, uint64_t end,uint64_t *key, uint64_t num)
{
    uint8_t calculatedHash[32];
    for (uint64_t i = start + num; i < end; i += 6)
    {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, &i, sizeof(i));
        SHA256_Final(calculatedHash, &sha256);
        if (memcmp(hash, calculatedHash, 32) == 0)
        {
            *key = i;
            return i;
        }
    }
    return 0;
}

// Priority Queue Node structure
typedef struct node
{
    request_t request;
    struct node *next;
} node_t;

node_t *head = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Function to create a new node
node_t *newNode(request_t request)
{
    node_t *temp = (node_t *)malloc(sizeof(node_t));
    temp->request = request;
    temp->next = NULL;
    return temp;
}

// Function to enqueue a request based on priority
void enqueue(request_t request)
{
    pthread_mutex_lock(&queue_mutex);

    node_t *new_node = newNode(request);
    if (head == NULL || head->request.prio > request.prio)
    {
        // Insert at head if queue is empty or new node has higher priority
        new_node->next = head;
        head = new_node;
    }
    else
    {
        // Find position for new node
        node_t *start = head;
        while (start->next != NULL && start->next->request.prio <= request.prio)
        {
            start = start->next;
        }
        new_node->next = start->next;
        start->next = new_node;
    }

    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Function to dequeue a request with the highest priority (integrated peek and removal)
request_t dequeue()
{
    pthread_mutex_lock(&queue_mutex);

    // Wait if queue is empty
    while (head == NULL)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Peek at the request with the highest priority (head)
    request_t request = head->request;

    // Remove the node after peeking
    node_t *temp = head;
    head = head->next;
    free(temp);

    pthread_mutex_unlock(&queue_mutex);
    return request;
}

// Worker thread function
void *worker(void *arg)
{
    int threadID = (int)(intptr_t)arg;
    int64_t oldRequestHash;
    while (1)
    {       

        if(threadID == 6){
            // Get the next request from the queue
            request_t request = dequeue();
            currentRequest = request;
            key = 0;
            // Check the cache before performing brute-force search
            if (checkCache(request.hash, &key))
            {
                // Key found in cache, no need to brute-force
                printf("Cache hit for hash\n");
            }

            while (key != 0)
            {

            }
                
            addToCache(request.hash, key);
            

            // Send back found key to client
            key = htobe64(key);
            write(request.client_socket, &key, 8);

            // Close the client socket
            close(request.client_socket);
        }else{
            request_t request = currentRequest;
            if (oldRequestHash == request.hash)
            {
                continue;
            }else
            {
                bruteForceSearch1(request.hash, request.start, request.end, &key, threadID);
                oldRequestHash = request.hash;
            }

        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
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

    // Listen for client connections
    listen(server_fd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int cli_length = sizeof(cli_addr);

    // Create worker threads
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, worker, (void *)(intptr_t)i);
    }

    // pthread_create(&threads[0], NULL, worker, NULL);
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
        request_t request = pack(buffer, client_socket);
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
            enqueue(request); // Enqueue based on priority
        }
    }

    return 0;
}
