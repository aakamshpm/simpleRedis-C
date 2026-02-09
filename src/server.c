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