#include <app.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
// Function to create a socket and bind it to the listen port
struct connection *createServer(int port, struct config *config) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error creating socket");
		exit(1);
	}

	// Set timeout value for the socket
	struct timeval tv;
	tv.tv_sec = config->timeout / 1000;
	tv.tv_usec = (config->timeout % 1000) * 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

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
	bool first = true;
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
		client_conn->addrlen = sizeof(client_conn->address);
		if (first) {
			first = false;
			puts("Waiting for connection...");
		}
		client_conn->sock = accept(conn->sock, (struct sockaddr *) &client_conn->address, &client_conn->addrlen);
		if (client_conn->sock < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connections, continue the loop
                continue;
			}
			perror("Error accepting connection");
			free(client_conn);
			continue;
		}
		puts("Got connection!");
		first = true;

		handleConnection(client_conn, config);
	}
}

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
	puts("Successful handshake");

	// Handle commands
	while (1) {
		if (recv(conn->sock, msg, sizeof(msg), 0) <= 0) {
			break;
		}
		if (strcmp(msg, "sync") == 0) {
			// Handle sync command
			if (config->mode == MODE_SERVER) {
				puts("Syncing...");
				system("task sync");
				puts("Sync complete");
				if (send(conn->sock, "OK", strlen("OK") + 1, 0) < 0) {
					perror("Error sending sync response");
					close(conn->sock);
					free(conn);
					return;
				}
			}
			else if (config->mode == MODE_GATEWAY) {
				for (int i = 0; i < config->num_servers; i++) {
					printf("Syncing to %s\r\n", config->server_ips[i]);
					if (!sendSyncCommand(config->server_ips[i], config->port, false)) {
						// TODO: Log the failure somewhere permanent.
						fprintf(stderr, "Error syncing to %s\r\n", config->server_ips[i]);

						continue;
					}
					if (recv(conn->sock, msg, sizeof(msg), 0) <= 0) {
						perror("Error receiving sync response");
						// close(conn->sock);
						// free(conn);
						// return;
					}
					if (strcmp(msg, "OK") != 0) {
						fprintf(stderr, "Error syncing: %s\r\n", msg);
					}
				}
			}
		}
		else {
			fprintf(stderr, "Invalid command: %s\n", msg);
			break;
		}
	}

	close(conn->sock);
	free(conn);
}

// Function to create a client connection to a server and send a "sync" command
bool sendSyncCommand(const char *server_ip, int port, bool fatal) {
	puts("Sending sync command");
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error creating socket");
		if (fatal) {
			return false;
		}
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_ip);
	address.sin_port = htons((uint16_t)port);

	if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("Error connecting to server");
		close(sock);
		if (fatal) {
			return false;
		}
	}

	if (send(sock, "HELLO " VERSION, strlen("HELLO " VERSION) + 1, 0) < 0) {
		perror("Error sending handshake");
		close(sock);
		if (fatal) {
			return false;
		}
	}

	char msg[MAX_MSG_LEN];
	if (recv(sock, msg, sizeof(msg), 0) < 0) {
		perror("Error receiving handshake");
		close(sock);
		if (fatal) {
			return false;
		}
	}
	if (strcmp(msg, "HELLO " VERSION) != 0) {
		fprintf(stderr, "Invalid handshake: %s\n", msg);
		close(sock);
		if (fatal) {
			return false;
		}
	}

	if (send(sock, "sync", strlen("sync") + 1, 0) < 0) {
		perror("Error sending sync command");
		if (fatal) {
			return false;
		}
	}

	close(sock);
	return true;
}
