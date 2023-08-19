#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>


static inline void cleanupServerIPs(struct config *config) {
	for (int i = 0; i < config->num_servers; i++) {
		free(config->server_ips[i]);
	}
	free(config->server_ips);
	free(config->gateway_ip);
}

int main(int argc, char *argv[]) {
	int ret = 0;
	struct config *config = readConfig("config.txt");
	
	if (config == NULL) {
		fputs("Config is null.... what\r\n", stderr);
		exit(ERR_CONFIG);
	}
	if (config->mode == 0xAA) {
		if (argc != 2) {
			// never got set in config, and not on the command line.  How are we supposed to know what mode to run in???
			fprintf(stderr, "Usage: %s (mode)\r\nPlease see config.txt for `mode` options.\r\nYou can specify a mode either in the config, or on the command line, but you must have at least 1.\r\n", argv[0]);
			exit(ERR_WRONG_MODE);
		}
		// not in config, but we got it on the cmdline!
		config->mode = atoi(argv[1]);
	}
	if (config->num_servers == 0) {
		fputs("No servers configured!  Aborting.\r\n", stderr);
		exit(ERR_CONFIG);
	}

	printConfig(config);

	if (config->mode == MODE_CLIENT) {
		ret = 0;
		if (!sendSyncCommand(config->gateway_ip, config->port, true)) {
			ret = ERR_SYNC;
		};
		goto leave;
	}
	struct connection *conn = createServer(config->port, config);
	startServer(conn, config);
	free(conn);

leave:
	cleanupServerIPs(config);
	free(config);
	return ret;
}