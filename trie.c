#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"

struct htt_trie *htt_trie_create(void) {
	return calloc(1, sizeof(struct htt_trie));
}

void htt_trie_destroy(struct htt_trie *t) {
	if (!t) return;
	for (int i = 0; i < 8; ++i) htt_trie_destroy(t->children[i]);
	free(t->value);
	free(t);
}

// generate a valid index from an ASCII character. lowercase characters will share an index with uppercase characters.
static inline int ctoindex(int c) {
	if (c < 32 || c > 127) return -1;
	c -= 32;
	if (c >= 64) c -= 32;
	return c;
}

char *htt_trie_insert(struct htt_trie *t, const char *key, char *value) {
	int i;
	while ((i = ctoindex(*key)) != -1) {
		// index is split into two 3-bit integers
		for (int j = 2; j > 0; --j) {
			// this looks so stupid but I cannot think of a better way to do this.
			struct htt_trie **child = &t->children[i & 7];
			t = *child = *child ? *child : htt_trie_create();
			if (!t) return NULL;
			++key;
			i >>= 3;
		}
	}
	
	t->value = value;
	return t->value;
}

char *htt_trie_search(struct htt_trie *t, const char *key) {
	int i;
	while ((i = ctoindex(*key)) != -1) {
		for (int j = 2; j > 0; --j) {
			t = t->children[i & 7];
			if (!t) return NULL;
			++key;
			i >>= 3;
		}
	}
	
	return t->value;
}
