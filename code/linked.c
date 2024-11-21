#include "funcs.h"

// Priority Queue Node structure
typedef struct node
{
    request_t request;
    struct node *next;
} node_t;

node_t *head = NULL;
node_t *tail = NULL;

// Function to create a new node
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
        // Queue is empty, insert at the head
        head = new_node;
    }
    else if (request.prio == 1)
    {
        // Priority 1: Directly append at the tail
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
    serv_addr.sin_port = htons(5000);

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
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

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
            key = htobe64(key);
            write(request.client_socket, &key, 8);
            close(request.client_socket);
        }
        else
        {
            enqueue(request); // Enqueue based on priority
        }
    }

    return 0;
}
