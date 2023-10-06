#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "trie.h"

static struct htt_trie *mime_type_trie;

void load_mime_type_list(void) {
	mime_type_trie = htt_trie_create();
	FILE *fp = fopen(global_config.mime_type_path, "r");
	if (!fp) {
		fprintf(stderr, "Failed to open MIME type list: %s\n", strerror(errno));
		exit(1);
	}
	
	char *key, *value;
	while (fscanf(fp, "%m[^=]=%ms\n", &key, &value) == 2) {
		htt_trie_insert(mime_type_trie, key, value);
		free(key);
	}
	
	fclose(fp);
}

// Returns the extension of a file (if there is one)
const char *get_file_ext(const char *path) {
	size_t len = strlen(path);
	const char *ext = path + len;
	while (ext != path && *ext != '.') --ext;
	return ext++ != path ? ext : NULL;
}

// Looks up the mime type associated with a file extension
// Returns the mime type on success, or NULL if it is unknown.
const char *lookup_mime_type(const char *ext) {
	return htt_trie_search(mime_type_trie, ext);
}
