#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h> // malloc(), free(), exit()
#include <string.h> // memset(), strerror()
#include <unistd.h>
#include <errno.h> // to capture error codes from system calls
#include <sys/socket.h> // functions to establish socket connection, accept clients, bind and listen.
#include <sys/select.h> // select() system call to listen client changes
#include <netinet/in.h> // provides sockaddr_in struct to store server and client network details
#include <arpa/inet.h> // inet_ntoo(): to convert IP addresses into readable strings
#include <fcntl.h> // file controls: to set sockets to Non Blocking mode using fcntl()

#define DEFAULT_PORT 6379
#define MAX_CLIENTS 1024 // max number of concurrent users server will track
#define BUFFER_SIZE 4096 // size of chunk data we read one at a time
#define BACKLOG 128 // number of pending connection OS queue will hold untill the server accepts the,

// Client state
typedef struct {
    int fd; // client file descriptor
    // in linux, every network connection is treated like a file, fd acts as the ID for each client

    char read_buffer[BUFFER_SIZE]; // memory space for each client to store data
    size_t read_pos; // used to track current posistion where we are writing in a buffer
    int active; // 1 or 0: if the value is 0, then array is empty and this fd can be reused for a new user
} client_t;

// Server State
typedef struct {
    int server_fd; // listens only for new clients, it doesn't transfer any data
    int port; // stores port number for server
    client_t clients[MAX_CLIENTS]; // array storing the state of every client
    int max_fd; // used in select(). it tells OS the highest FD we have right now, so select() knows how far it should listen
} server_t;

// function declarations
server_t* server_create(int port);

#endif