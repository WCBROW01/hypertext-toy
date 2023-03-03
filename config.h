/**
 * @file config.h
 * @author Will Brown
 * @brief Structures and functions for server configuration
 * @version 0.1
 * @date 2023-03-03
 * 
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#include <netinet/in.h>

#include "constants.h"

/**
 * @brief struct containing the configuration of the server
 */
struct server_config {
    char root_path[HTTP_PATH_MAX];
    size_t root_path_len;
    in_port_t server_port;
};

extern struct server_config global_config;

/**
 * @brief Load a reasonable set of defaults for testing purposes
 * 
 * @return 0 on success, current state of errno on failure
 */
int load_default_config(void);

/**
 * @brief Save the configuration settings to a file at the given path
 * 
 * @param path the path to save at
 * @return 0 on success, current state of errno on failure
 */
int save_config(const char *path);

/**
 * @brief Load the configuration settings from a file at the given path
 * 
 * @param path the path to load from
 * @return 0 on success, current state of errno on failure
 */
int load_config(const char *path);

#endif // CONFIG_H