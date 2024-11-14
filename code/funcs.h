
#define NUM_THREADS 7
#define CACHE_SIZE 1000
#define QUEUE_SIZE 1000 // Array size for the priority queue

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

// Request structure for client data
typedef struct
{
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t prio;
    int client_socket;
} request_t;

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
