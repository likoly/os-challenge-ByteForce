#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "messages.h"


#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

void doprocessing (int sock) {
   int n;
   uint8_t buffer[256];
   bzero(buffer,256);
   n = read(sock,buffer,255);
   
   if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }
   
   printf("Here is the message: %s\n",buffer);
   //  uint64_t value =  htobe64(uint64_t host_64bits);

  

   uint64_t value;
   memcpy(&value, &buffer[PACKET_REQUEST_HASH_OFFSET ], sizeof(uint64_t));
   value =  htobe64(value);
   printf("here is the number: %" PRIu64 "\n",value);

   uint64_t start;
   memcpy(&start, &buffer[PACKET_REQUEST_START_OFFSET ], sizeof(uint64_t));
   start =  htobe64(start) - 1 ;
   printf("Start: %" PRIu64 "\n",start);


   uint64_t end;
   memcpy(&end, &buffer[PACKET_REQUEST_END_OFFSET ], sizeof(uint64_t));
   end =  htobe64(end) - 1;
   printf("End: %" PRIu64 "\n",end);

   
   // uint8_t intial_number_sent;
   // intial_number_sent = buffer[]  
   //printf
   n = write(sock,"I got your message",18);
   
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