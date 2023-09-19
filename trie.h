#ifndef HTT_TRIE_H
#define HTT_TRIE_H

// trie supports ASCII alphanumeric + special characters, case insensitive
struct htt_trie {
	struct htt_trie *children[64];
	char *value;
};

struct htt_trie *htt_trie_create(void);

void htt_trie_destroy(struct htt_trie *t);

// this will transfer ownership of the value string to the trie.
char *htt_trie_insert(struct htt_trie *t, const char *key, char *value);

char *htt_trie_search(struct htt_trie *t, const char *key);

#endif // HTT_TRIE_H
