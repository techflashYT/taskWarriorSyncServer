#include <app.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
// Function to create a socket and bind it to the listen port
struct connection *createServer(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        exit(1);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)port);

    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    struct connection *conn = (struct connection *)malloc(sizeof(struct connection));
    if (conn == NULL) {
        perror("Error allocating memory for connection");
        exit(1);
    }
    conn->sock = sock;
    conn->address = address;
    conn->addrlen = sizeof(address);

    return conn;
}

static void handleConnection(struct connection *conn, struct config *config);

// Function to start a loop to accept connections on a socket
void startServer(struct connection *conn, struct config *config) {
    if (listen(conn->sock, 5) < 0) {
        perror("Error listening on socket");
        exit(1);
    }

    while (1) {
        struct connection *client_conn = malloc(sizeof(struct connection));
        if (client_conn == NULL) {
            perror("Error allocating memory for client connection");
            continue;
        }
        client_conn->sock = accept(conn->sock, (struct sockaddr *) &client_conn->address, &client_conn->addrlen);
        if (client_conn->sock < 0) {
            perror("Error accepting connection");
            free(client_conn);
            continue;
        }

        handleConnection(client_conn, config);
    }
}

// Function to perform the handshake and handle commands
static void handleConnection(struct connection *conn, struct config *config) {
        char msg[MAX_MSG_LEN];

    // Perform the handshake
    if (recv(conn->sock, msg, sizeof(msg), 0) < 0) {
        perror("Error receiving handshake");
        close(conn->sock);
        free(conn);
        return;
    }
    if (strcmp(msg, "HELLO " VERSION) != 0) {
        fprintf(stderr, "Invalid handshake: %s\n", msg);
        close(conn->sock);
        free(conn);
        return;
    }
    if (send(conn->sock, "HELLO " VERSION, strlen("HELLO " VERSION) + 1, 0) < 0) {
        perror("Error sending handshake");
        close(conn->sock);
        free(conn);
        return;
    }

    // Handle commands
    while (1) {
        if (recv(conn->sock, msg, sizeof(msg), 0) <= 0) {
            break;
        }
        if (strcmp(msg, "sync") == 0) {
            // Handle sync command
            if (config->mode == MODE_SERVER) {
                system("task sync");
            }
            else if (config->mode == MODE_GATEWAY) {
                for (int i = 0; i < config->num_servers; i++) {
                    sendSyncCommand(config->server_ips[i], config->port);
                }
            }
        } else {
            fprintf(stderr, "Invalid command: %s\n", msg);
            break;
        }
    }

    close(conn->sock);
    free(conn);
}

// Function to create a client connection to a server and send a "sync" command
void sendSyncCommand(const char *server_ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons((uint16_t)port);

    if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Error connecting to server");
        close(sock);
        return;
    }

    if (send(sock, "HELLO " VERSION, strlen("HELLO " VERSION) + 1, 0) < 0) {
        perror("Error sending handshake");
        close(sock);
        return;
    }

    char msg[MAX_MSG_LEN];
    if (recv(sock, msg, sizeof(msg), 0) < 0) {
        perror("Error receiving handshake");
        close(sock);
        return;
    }
    if (strcmp(msg, "HELLO " VERSION) != 0) {
        fprintf(stderr, "Invalid handshake: %s\n", msg);
        close(sock);
        return;
    }

    if (send(sock, "sync", strlen("sync") + 1, 0) < 0) {
        perror("Error sending sync command");
    }

    close(sock);
}
