#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

// App version
#define VERSION "1.0"

// App modes
#define MODE_CLIENT 1
#define MODE_SERVER 2
#define MODE_GATEWAY 3

// Maximum length of a message
#define MAX_MSG_LEN 256

// Maximum config line length
#define MAX_CONFIG_LINE_LEN 512

// Error codes
#define ERR_INVALID_CMD 1
#define ERR_TIMEOUT 2
#define ERR_WRONG_MODE 3
#define ERR_CONFIG 4

// Config structure
struct config {
	int mode;
	int port;
	int timeout;
	char **server_ips;
	int num_servers;
	char *gateway_ip;
};

// Connection structure
struct connection {
	int sock;
	struct sockaddr_in address;
	socklen_t addrlen;
};


extern struct config *readConfig(const char *filename);
// Function to create a socket and bind it to the listen port
extern struct connection *createServer(int port, struct config *config);
extern void startServer(struct connection *conn, struct config *config);
extern bool sendSyncCommand(const char *server_ip, int port, bool fatal);