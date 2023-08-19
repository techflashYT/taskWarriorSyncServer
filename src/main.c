#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>

int main() {
	struct config *config = readConfig("config.txt");
	(void)config;

	char *modeStr;
	if (config->mode == MODE_CLIENT) {
		modeStr = "client";
	}
	else if (config->mode == MODE_SERVER) {
		modeStr = "server";
	}
	else if (config->mode == MODE_GATEWAY) {
		modeStr = "gateway";
	}

	char *serverIpsStr = malloc(2048);
	strcpy(serverIpsStr, "[ ");
	for (int i = 0; i < config->num_servers; i++) {
		strcat(serverIpsStr, config->server_ips[i]);
		strcat(serverIpsStr, ", ");
	}
	if (serverIpsStr[strlen(serverIpsStr) - 2] == ',') {
		serverIpsStr[strlen(serverIpsStr) - 2] = '\0';
	};
	strcat(serverIpsStr, " ]");
	printf(
		"=== Config ===\r\n"
		"mode: %s\r\n"
		"port: %d\r\n"
		"timeout: %d\r\n"
		"server_ips: %s\r\n"
		"gateway_ip: %s\r\n"
		"=== End Config ===\r\n",
		modeStr,
		config->port,
		config->timeout,
		serverIpsStr,
		config->gateway_ip
	);
	if (config->num_servers == 0) {
		fputs("No servers configured!  Aborting.\r\n", stderr);
		exit(ERR_CONFIG);
	}
	free(serverIpsStr);
	if (config->mode == MODE_CLIENT) {
		sendSyncCommand(config->gateway_ip, config->port);
	}
	struct connection *conn = createServer(config->port);
	startServer(conn, config);


	free(config);
	free(conn);
}