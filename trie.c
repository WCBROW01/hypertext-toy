#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"

struct htt_trie *htt_trie_create(void) {
	return calloc(1, sizeof(struct htt_trie));
}

void htt_trie_destroy(struct htt_trie *t) {
	if (!t) return;
	for (int i = 0; i < 63; ++i) htt_trie_destroy(t->children[i]);
	free(t->value);
	free(t);
}

// make 
static inline int ctoindex(int c) {
	if (c < 32 || c > 127) return -1;
	c -= 32;
	if (c >= 64) c -= 32;
	return c;
}

char *htt_trie_insert(struct htt_trie *t, const char *key, char *value) {
	int i;
	while ((i = ctoindex(*key)) != -1) {
		// this looks so stupid but I cannot think of a better way to do this.
		struct htt_trie **child = &t->children[i];
		t = *child = *child ? *child : htt_trie_create();
		if (!t) return NULL;
		++key;
	}
	
	t->value = value;
	return t->value;
}

char *htt_trie_search(struct htt_trie *t, const char *key) {
	int i;
	while ((i = ctoindex(*key)) != -1) {
		t = t->children[i];
		if (!t) return NULL;
		++key;
	}
	
	return t->value;
}
