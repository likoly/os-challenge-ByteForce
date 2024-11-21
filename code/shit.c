
// // Worker thread function
// void *worker(void *arg)
// {
//     while (1)
//     {
//         // Get the next request from the queue
//         request_t request = dequeue();

//         uint64_t key = 0;
//         // Check the cache before performing brute-force search
//         if (checkCache(request.hash, &key))
//         {
//             // Key found in cache, no need to brute-force
//             printf("Cache hit for hash\n");
//         }
//         else
//         {

//             key = bruteForceSearch(request.hash, request.start, request.end);
//             if (key != 0)
//             {
//                 addToCache(request.hash, key);
//             }
//         }

//         // Send back found key to client
//         key = htobe64(key);
//         write(request.client_socket, &key, 8);

//         // Close the client socket
//         close(request.client_socket);
//     }
//     return NULL;
// }

// // Worker thread function
// void *worker(void *arg)
// {
//     while (1)
//     {
//         // Get the next request from the queue
//         request_t request = dequeue();

//         uint64_t key = 0;
//         // Check the cache before performing brute-force search
//         if (checkCache(request.hash, &key))
//         {
//             // Key found in cache, no need to brute-force
//             printf("Cache hit for hash\n");
//         }
//         else
//         {
//             key = bruteForceSearch(request.hash, request.start, request.end);
//             if (key != 0)
//             {
//                 addToCache(request.hash, key);
//             }
//         }

//         // Send back found key to client
//         key = htobe64(key);
//         write(request.client_socket, &key, 8);

//         // Close the client socket
//         close(request.client_socket);
//     }
//     return NULL;
// }

// // Worker thread function
// void *worker(void *arg)
// {
//     while (1)
//     {
//         request_t request = dequeue();
//         uint64_t key = 0;

//         if (checkCache(request.hash, &key))
//         {
//             printf("Cache hit for hash\n");
//         }
//         else
//         {
//             key = bruteForceSearch(request.hash, request.start, request.end);
//             if (key != 0)
//             {
//                 addToCache(request.hash, key);
//             }
//         }

//         key = htobe64(key);
//         write(request.client_socket, &key, sizeof(key));
//         close(request.client_socket);
//     }
//     return NULL;
// }

// // Worker thread function (same as before)
// void *worker(void *arg)
// {
//     while (1)
//     {
//         request_t request = dequeue();
//         uint64_t key = 0;

//         if (checkCache(request.hash, &key))
//         {
//             printf("Cache hit for hash\n");
//         }
//         else
//         {
//             key = bruteForceSearch(request.hash, request.start, request.end);
//             if (key != 0)
//             {
//                 addToCache(request.hash, key);
//             }
//         }

//         key = htobe64(key);
//         write(request.client_socket, &key, sizeof(key));
//         close(request.client_socket);
//     }
//     return NULL;
// }
