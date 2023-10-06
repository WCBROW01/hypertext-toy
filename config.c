#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <netinet/in.h>

#include "constants.h"
#include "config.h"

struct server_config global_config;

int load_default_config(void) {
    if (!getcwd(global_config.root_path, sizeof(global_config.root_path))) {
    	return errno;
    }

    global_config.root_path_len = strlen(global_config.root_path);
    global_config.max_age = 60; // temporary value
    global_config.mime_type_path = "mime-types.txt";
    global_config.flags = CONFIG_DIR_LISTING | CONFIG_COURTESY_REDIR;
    global_config.server_port = htons(8000);
    return 0;
}

int save_config(const char *path) {
    (void) path;
    assert(0 && "Not Implemented Yet");
}

int load_config(const char *path) {
    (void) path;
    assert(0 && "Not Implemented Yet");
}

int parse_config_option(const char *opt) {
	if (sscanf(opt, "root_path=%4096s", global_config.root_path) == 1) {
		global_config.root_path_len = strlen(global_config.root_path);
		return 1;
	}
	if (sscanf(opt, "mime_type_path=%ms", &global_config.mime_type_path) == 1) return 1;
	if (sscanf(opt, "max_age=%d", &global_config.max_age) == 1) return 1;
	
	char bool_opt[6];
	if (sscanf(opt, "dir_listing=%5s", bool_opt) == 1) {
		if (!strcmp(bool_opt, "true")) global_config.flags |= CONFIG_DIR_LISTING;
		else global_config.flags &= ~CONFIG_DIR_LISTING;
		return 1;
	}
	if (sscanf(opt, "courtesy_redir=%5s", bool_opt) == 1) {
		if (!strcmp(bool_opt, "true")) global_config.flags |= CONFIG_COURTESY_REDIR;
		else global_config.flags &= ~CONFIG_COURTESY_REDIR;
		return 1;
	}
	if (sscanf(opt, "server_port=%hu", &global_config.server_port) == 1) {
		global_config.server_port = htons(global_config.server_port);
		return 1;
	}
	
	return 0;
}
