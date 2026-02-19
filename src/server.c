#include "server.h"

server_t *server_create(int port) // socket that listens to client
{
    server_t *server = malloc(sizeof(server_t)); // allocates memory heap (RAM) for server struct
    if (!server)
    {
        perror("malloc failed");
        exit(1);
    }

    server->port = port;
    server->max_fd = 0; // initially there is no sockets(neither server nor client), so FD value would be 0

    // initialize all MAX_CLIENTS clients as inactive
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        // we mark MAX_CLIENTS (eg: 1024 clients) as inactive because when memory is allocated to clients,
        // some garbage value will be remaining for that client's heap(RAM) from previous random program.
        // this may caues the server to think the client slot is already taken or even try to read from a non existent socket and crash
        // this loop ensures empty seats for all available clients
        server->clients[i].fd = -1;
        server->clients[i].active = 0;
        server->clients[i].read_pos = 0;
        // the above steps ensure that this particular client is inactive
    }

    server->server_fd = socket(AF_INET, SOCK_STREAM, 0); // create server socket
    if (server->server_fd < 0)
    {
        perror("server socket failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        // if we crash the server and restart it immediatly, the OS usually holds the port for 60seconds (TIME_WAIT state)
        // the SO_REUSEADDR ensures OS release the port immediately so that we can restart server in an instant
        // without SO_REUSEADDR, program will throw 'bind() failed: Address already in use' error.
        perror("setsocketopt failed");
        exit(1);
    }

    struct sockaddr_in addr; // address structure to store server network details
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;         // IPV4
    addr.sin_addr.s_addr = INADDR_ANY; // accepts clients from any address (0.0.0.0)
    addr.sin_port = htons(port);       // converts port number to network byte order

    // bind (glues the socket to the specific IP and port configured above)
    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind failed");
        exit(1);
    }

    if (listen(server->server_fd, BACKLOG) < 0) // switches server to passive mode, where it waits for incoming calls
    {
        perror("listen failed");
        exit(1);
    }

    server->max_fd = server->server_fd; // since server socket is the only one open, it is currently the highest file descriptor

    printf("Server created on port %d\n", port);
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Listening on 0.0.0.0:%d\n", port);

    return server;
}

void server_run(server_t *server)
{
    printf("Server running. Waiting for connections... \n");

    while (1) // ensures this is an infinite loop where server runs forever
    {
        fd_set read_fds; // fd_set is data structure that holds file descriptors

        FD_ZERO(&read_fds); // clears the set

        // First, add server socket to the set (to detect new client connections)
        FD_SET(server->server_fd, &read_fds);

        // next, add all active client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (server->clients[i].active)
            {
                FD_SET(server->clients[i].fd, &read_fds);
            }
        }

        // select() monitors all FDs in read_fds
        // it blocks (sleeps) until at least one FD has data ready (kernel checks for data change)
        // returns number of FDs that are ready
        int activity = select(server->max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted by signal, retry
            }
            perror("select() failed");
            break;
        }

        // check if server socket has activity (if server has activity, that means a new client wants to connect)
        if (FD_ISSET(server->server_fd, &read_fds))
        {
            server_accept_client(server);
        }

        // check all clients for activity (existing clients sending data)
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (server->clients[i].active && (server->clients[i].fd, &read_fds))
            {
                server_read_client(server, i); // passes both server instance and client position
            }
        }
    }
}

// set each FD to non-blocking mode
// by default socket reading is done in blocking mode
// server will keep on waiting for a data to arrive from client
// in non-blocking mode, server would instantly return and listen for data from another client if no data was found in one client
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    // fcntl -> File Control: it's used to modify FD
    // fd: the File Descriptor we want to modify
    // F_GETFL: Retreive the current settings for this socket
    // we do this because, when we add a new setting, we don't want to loss the old ones accidently

    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    // F_SETFL: used to set a new setting to FD
    // flags | O_NONBLOCK : we combine the current settings of that FD with Non Blocking Mode
    // Old Settings + Non-Blocking mode
    {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }

    return 0;
}

// accept a new client connection
void server_accept_client(server_t *server)
{
    struct sockaddr_in client_addr; // a blank clipboard which we will use to store incoming client details
    // client_addr is a chunk of memory big enough to store an IPv4 address and a port number.
    // initially it will be filled with garbage values

    socklen_t addr_len = sizeof(client_addr); // the size of blank clipboard. we note down this to ensure we don't write outside the specified memory region

    int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr, &addr_len);
    // incoming client data will be stored on client_addr
    // after accept() is done, the addr_len will be updated with written data size in bytes

    if (client_fd < 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN) // we log all client failure's except EWOULDBLOCK and EAGAIN
        {
            perror("accept() failed");
        }
        return;
        // sometimes the client sends request before server spins up, hence the request maybe lost due to time out.
        // such scenarios are often and this would be considered as an EWOULDBLOCK and EAGAIN errors.
        // we don't need to spam connection failure logs for this scenario, hence we ignore them and return
    }

    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!server->clients[i].active)
        {
            slot = i;
            break;
            // we iterate through the clients and find an inactive field for new client
        }
    }

    if (slot == -1)
    {
        printf("Max clients reached, rejecting connection\n");
        close(client_fd);
        return;
    }

    set_nonblocking(client_fd); // set the client as non-blocking

    // initialize client
    server->clients[slot].fd = client_fd;                      // assign FD
    server->clients[slot].active = 1;                          // mark the seat as active
    server->clients[slot].read_pos = 0;                        // start reading from top
    memset(server->clients[slot].read_buffer, 0, BUFFER_SIZE); // wipe the memory buffer clean with zeros, so that previous data left by old client doesn't get mixed up with new

    printf("Client connected: %s:%d (fd=%d, slot=%d)\n",
           inet_ntoa(client_addr.sin_addr), // converts binary IP address to text
           ntohs(client_addr.sin_port),     // converts network byte order to default port number
           client_fd, slot);
}