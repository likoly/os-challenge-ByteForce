#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "messages.h"



#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

// void hashe(){
//    uint64_t ans = (uint64_t) 1;
//    ans = (uint64_t) htole64(ans);
//    const uint64_t *num = 1;
   
//    // Buffer to store the SHA-256 digest
//    unsigned char hash[SHA256_DIGEST_LENGTH];
   
//    // Compute the SHA-256 hash
//    SHA256((uint64_t*)num, sizeof(uint64_t), hash);
   
//    // Print the resulting hash in hexadecimal
//    printf("SHA-256 hash of \"%s\":\n", hash);
// }
void uint8_array_to_hex(const uint8_t *array, size_t len) {
    for (size_t i = 0; i < len; i++) {
      //   sprintf(output + (i * 2), "%02x", array[i]);
        printf( "%02x", array[i]);
    }
   //  output[len * 2] = '\0';  // Null-terminate the output string
}
void doprocessing (int sock) {
   int n;
   // uint8_t buffer[256];
   // PACKET_REQUEST_SIZE
   unsigned char buffer[256];
   bzero(buffer,256);
   n = read(sock,buffer,255);
   
   if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }
   
   // printf("Here is the message: %s\n",buffer);
   //  uint64_t value =  htobe64(uint64_t host_64bits);

  
   // htobe64, htole64, be64toh, and le64toh
   uint8_t value[32];
   printf("here is the hash: \n");
   // memcpy(&value, &buffer[PACKET_REQUEST_HASH_OFFSET ], sizeof(uint8_t)*32);
   for (size_t i = 0; i < 32; i++)
   {
      
   memcpy(&value[31-i], &buffer[PACKET_REQUEST_HASH_OFFSET + 31 - i ], sizeof(uint8_t));
   // value[i] = htole64(value[i]);
   // printf("%"PRIu8, value[i] );
   }
   uint8_array_to_hex(value,32);
     printf(" \n"); 
   // uint64_t val[4];  
   // for (size_t i = 0; i < 4; i++)
   // {
   //    memcpy(&val[i], &buffer[PACKET_REQUEST_HASH_OFFSET + i ], sizeof(uint64_t));
   //    printf("%"PRIu64 "\n", val[i] );  
   // }
// printf(" \n");
// printf(" \n");   
  



   for (size_t i = 0; i < 32; i++)
   {
      
      printf("%" PRIu8 "",value[i]);
   // value[i] = htole64(value[i]);
   // printf("%"PRIu8, value[i] );
   }
    printf(" \n");
   // printf("%"PRIu64, value );
   // printf("value is %ld", value);
   // printf("value array is: %ld \n", sizeof(value));


   // for (size_t i = 0; i < 32; i++)
   // {
      
   // memcpy(&value[i], &buffer[PACKET_REQUEST_HASH_OFFSET + i ], sizeof(uint8_t));
   // // value[i] = htole64(value[i]);
   // // printf("%"PRIu8, value[i] );
   // }
   //  printf(" \n");
   // printf("%"PRIu64, value );

   // // value =  htobe64(value);
  
   uint64_t start;
   memcpy(&start, &buffer[PACKET_REQUEST_START_OFFSET -1], sizeof(uint64_t));
   start =  htobe64(start) - 1 ;
   printf("Start: %" PRIu64 "\n",start);

   uint64_t end;
   memcpy(&end, &buffer[PACKET_REQUEST_END_OFFSET ], sizeof(uint64_t));
   end =  htobe64(end) - 1;
   printf("End: %" PRIu64 "\n",end);
   
   uint8_t prio;
   memcpy(&prio, &buffer[PACKET_REQUEST_PRIO_OFFSET ], sizeof(uint8_t));
   // prio =  htobe64(prio) - 1;
   printf("Prio: %" PRIu8 "\n",prio);

   printf(" donezo\n\n");
   // uint8_t intial_number_sent;
   // intial_number_sent = buffer[]  
   //printf

   uint64_t ans = (uint64_t) 1;
   
   ans = htobe64(ans); 
   
   n = write(sock,&ans,PACKET_RESPONSE_SIZE);
   
   if (n < 0) {
      perror("ERROR writing to socket");
      exit(1);
   }
	
}

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen;
   char buffer[PACKET_REQUEST_SIZE];
   struct sockaddr_in serv_addr, cli_addr;
   int n, pid;
   // hashe();
   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = 5003;
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }
   
   /* Now start listening for the clients, here
      * process will go in sleep mode and will wait
      * for the incoming connection
   */
   
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   while (1) {
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		
      if (newsockfd < 0) {
         perror("ERROR on accept");
         exit(1);
      }
      
      /* Create child process */
      pid = fork();
		
      if (pid < 0) {
         perror("ERROR on fork");
         exit(1);
      }
      
      if (pid == 0) {
         /* This is the client process */
         close(sockfd);
         doprocessing(newsockfd);
         exit(0);
      }
      else {
         close(newsockfd);
      }
		
   } /* end of while */
}