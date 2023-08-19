#include <app.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Function to read the config file
struct config *readConfig(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		perror("Error opening config file");
		exit(ERR_CONFIG);
	}

	struct config *config = malloc(sizeof(struct config));
	if (config == NULL) {
		perror("Error allocating memory for config");
		exit(ERR_CONFIG);
	}
	// set default values
	config->mode = MODE_CLIENT;
	config->port = 8080;
	config->timeout = 5;
	config->server_ips = NULL;
	config->num_servers = 0;
	config->gateway_ip = NULL;

	char line[MAX_CONFIG_LINE_LEN];
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == '#' || line[0] == '\r' || line[0] == '\n' || line[0] == ' ') {
			// comment / whitespace
			continue;
		}
		char *key = malloc(32);
		int keyIndex = 0;
		for (int i = 0; i < 32; i++) {
			key[i] = line[i];
			if (line[i] == ' ' || line[i] == '=') {
				key[i] = '\0';
				keyIndex = i;
				break;
			}
		}
		char *value = malloc(256);
		for (int i = keyIndex + 3; i < 256; i++) {
			value[i - keyIndex - 3] = line[i];
			if (line[i] == '\r' || line[i] == '\n') {
				value[i - keyIndex - 3] = '\0';
				break;
			}
		}
		if (key == NULL || value == NULL) {
			fprintf(stderr, "Invalid config line: %s\n", line);
			exit(ERR_CONFIG);
		}

		if (strcmp(key, "mode") == 0) {
			config->mode = atoi(value);
		}
		else if (strcmp(key, "port") == 0) {
			config->port = atoi(value);
		}
		else if (strcmp(key, "timeout") == 0) {
			config->timeout = atoi(value);
		}
		else if (strcmp(key, "server_ips") == 0) {
			char *ip = strtok(value, ",");
			while (ip != NULL) {
				// ship whitespace
				while (*ip == ' ' || *ip == '\t') {
					ip++;
				}
				config->server_ips = realloc(config->server_ips, (config->num_servers + 1) * sizeof(char *));
				if (config->server_ips == NULL) {
					perror("Error allocating memory for server IPs");
					exit(ERR_CONFIG);
				}
				config->server_ips[config->num_servers] = strdup(ip);
				config->num_servers++;
				ip = strtok(NULL, ",");
			}
		}
		else if (strcmp(key, "gateway_ip") == 0) {
			config->gateway_ip = strdup(value);
		}
		else {
			fprintf(stderr, "Unknown config key: %s\n", key);
			exit(ERR_CONFIG);
		}
		free(key);
		free(value);
	}

	fclose(file);
	return config;
}
