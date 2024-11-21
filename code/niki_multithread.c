#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>
#include <pthread.h>

#include "code/messages.h"

// NOTE: Used https://www.tutorialspoint.com/unix_sockets/client_server_model.htm to understand and build our socket logic

// 
void* reverseHash(void *newsockfdPtr) {

    // 
    int newsockfd = *(int*)newsockfdPtr;
    free(newsockfdPtr);

    // 
    uint8_t buffer[PACKET_REQUEST_SIZE];
    read(newsockfd, buffer, PACKET_REQUEST_SIZE);

    // 
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t p;
    memcpy(hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
    memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, 8);
    memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, 8);
    memcpy(&p, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);

    // 
    start = htobe64(start);
    end = htobe64(end);

    // 
    uint8_t calculatedHash[32];
    uint64_t key;
    for (key = start; key < end; key++) {
        SHA256((uint8_t *)&key, 8, calculatedHash);
        if (memcmp(hash, calculatedHash, 32) == 0)
            break;
    }

    // 
    key = be64toh(key);
    write(newsockfd, &key, 8);

    //
    close(newsockfd);
    pthread_exit(NULL);
}

// 
int main(int argc, char *argv[]) {

    //
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    //
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // 
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // 
    listen(sockfd, 100);

    // 
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // 
    while (1) {

        // 
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        // 
        int *newsockfdPtr = malloc(sizeof(int));
        memcpy(newsockfdPtr, &newsockfd, sizeof(int));

        // 
        pthread_t tid;
        pthread_create(&tid, NULL, reverseHash, newsockfdPtr);
    }
}
