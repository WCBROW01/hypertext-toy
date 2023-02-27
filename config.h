#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#include <netinet/in.h>

#include "constants.h"

struct server_config {
    char root_path[HTTP_PATH_MAX];
    size_t root_path_len;
    in_port_t server_port;
};

extern struct server_config global_config;

// Returns 0 on success, errno on failure.
int load_default_config(void);

int save_config(const char *path);
int load_config(const char *path);

#endif