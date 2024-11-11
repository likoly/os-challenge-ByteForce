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
#include "funcs.h" //has cache, bruteforce, pack, number ofthread structure of request

// Priority queue array and its size
request_t request_queue[QUEUE_SIZE];
int queue_size = -1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Function to get the index of the highest-priority request
int peek()
{
    int highestPriority = -1;
    int highestIndex = -1;

    for (int i = 0; i <= queue_size; i++)
    {
        if (request_queue[i].prio > highestPriority)
        {
            highestPriority = request_queue[i].prio;
            highestIndex = i;
        }
    }
    return highestIndex;
}

// Function to enqueue a request
void enqueue(request_t request)
{
    pthread_mutex_lock(&queue_mutex);
    if (queue_size < QUEUE_SIZE - 1)
    {
        queue_size++;
        request_queue[queue_size] = request;
    }
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Function to dequeue the highest-priority request
request_t dequeue()
{
    pthread_mutex_lock(&queue_mutex);

    // Wait if the queue is empty
    while (queue_size == -1)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    int highestIndex = peek();
    request_t highest_prio_request = request_queue[highestIndex];

    // Shift elements to remove the dequeued element
    for (int i = highestIndex; i < queue_size; i++)
    {
        request_queue[i] = request_queue[i + 1];
    }
    queue_size--;

    pthread_mutex_unlock(&queue_mutex);
    return highest_prio_request;
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

// Prio stats: 53 43 35 33 34 27 24 25 20 21 20 14 8 18 16 109
// Results: 100.00 83522363
