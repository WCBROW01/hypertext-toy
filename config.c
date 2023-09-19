#include <assert.h>
#include <stddef.h>
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
