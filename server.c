#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include "messages.h"
#include <string.h>

void *doprocessing(void *sock);
uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end);

// Structure to hold cached hash data
typedef struct
{
    uint64_t number;
    unsigned char hash[SHA256_DIGEST_LENGTH];
} HashCacheEntry;

HashCacheEntry cache[1000];
int cache_size = 0;
pthread_mutex_t cache_mutex; // Mutex to protect cache access

uint64_t find_cache(unsigned char target_hash[]);
void add_cache(unsigned char target_hash[], uint64_t num);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    pthread_mutex_init(&cache_mutex, NULL); // Initialize the mutex

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 5002;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind the host address using bind() call */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Start listening for clients */
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        // Create a new thread to handle each client
        pthread_t thread;
        int *newsockfd_ptr = malloc(sizeof(int));
        *newsockfd_ptr = newsockfd; // Pass the socket to the thread

        if (pthread_create(&thread, NULL, doprocessing, (void *)newsockfd_ptr) < 0)
        {
            perror("ERROR creating thread");
            exit(1);
        }

        // Detach the thread so that it cleans up after itself
        pthread_detach(thread);
    }

    // Destroy the mutex before exiting
    pthread_mutex_destroy(&cache_mutex);

    return 0;
}

void *doprocessing(void *sock)
{
    int newsockfd = *((int *)sock);
    free(sock); // Free the dynamically allocated socket pointer

    int n;
    unsigned char buffer[PACKET_REQUEST_SIZE]; // 32 (hash) + 8 (start) + 8 (end) + 1 (p)
    bzero(buffer, PACKET_REQUEST_SIZE);
    n = read(newsockfd, buffer, PACKET_REQUEST_SIZE); // Read the full request packet

    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    // Print the received hash in hexadecimal
    printf("Received hash:");
    for (size_t i = 0; i < PACKET_REQUEST_START_OFFSET; i++)
    {
        printf("%02x ", buffer[i]); // Print each byte of the hash
    }
    printf("\n");

    // Extract and print the 'start' value (next 8 bytes)
    uint64_t start;
    memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start)); // Copy from buffer into start
    start = be64toh(start);                                              // Convert from big-endian to host byte order
    // printf("Start: %llu\n", (unsigned long long)start);

    // Extract and print the 'end' value (next 8 bytes)
    uint64_t end;
    memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, sizeof(end)); // Copy from buffer into end
    end = be64toh(end);                                            // Convert from big-endian to host byte order
    // printf("End: %llu\n", (unsigned long long)end);

    uint64_t result = bruteForce(buffer, start, end); // runs brute force
    uint64_t num_net = htobe64(result);               // Convert to network byte order

    n = write(newsockfd, &num_net, sizeof(num_net)); // Sends answer and assigns "command" value to n
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(1);
    }

    close(newsockfd);
    return NULL;
}

uint64_t find_cache(unsigned char target_hash[])
{
    pthread_mutex_lock(&cache_mutex); // Lock mutex before accessing cache
    for (size_t i = 0; i < cache_size; i++)
    {
        if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            pthread_mutex_unlock(&cache_mutex); // Unlock mutex after access
            return cache[i].number;
        }
    }
    pthread_mutex_unlock(&cache_mutex); // Unlock mutex after access
    return 0;
}

void add_cache(unsigned char target_hash[], uint64_t num)
{
    pthread_mutex_lock(&cache_mutex); // Lock mutex before modifying cache
    cache[cache_size].number = num;
    memcpy(cache[cache_size].hash, target_hash, SHA256_DIGEST_LENGTH);
    cache_size++;

    // Print the current contents of the cache for debugging
    for (size_t i = 0; i < cache_size; i++)
    {
        // printf("REAL NUMBER %i Cache size %i Number: %llu, Hash: ", i, cache_size, cache[i].number);
        for (size_t j = 0; j < SHA256_DIGEST_LENGTH; j++)
        {
            // printf("%02x", cache[i].hash[j]); // Print the hash as a hexadecimal string
        }
        // printf("\n");
    }
    pthread_mutex_unlock(&cache_mutex); // Unlock mutex after modification
}

uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    uint64_t a = find_cache(target_hash);
    if (a > 0)
    {
        printf("Match found for number: %llu in cache\n", a);
        return a;
    }

    for (uint64_t num = start; num < end; num++) // try from start to end
    {
        SHA256((unsigned char *)&num, sizeof(num), hash);         // does SHA256 on num, and returns it in hash
        if (memcmp(hash, target_hash, SHA256_DIGEST_LENGTH) == 0) // checking if hash == target hash
        {
            printf("Match found for number: %llu\n", num);
            add_cache(hash, num);
            return num;
        }
    }

    printf("No match found.\n");
    return -1;
}
