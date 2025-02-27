#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// https://nullprogram.com/blog/2023/09/30/
// https://github.com/skeeto/scratch/blob/master/misc/concurrent-hash-trie.c

#include <stli/allocator.h>

typedef struct HashTrieNode HashTrieNode;

#define HASH_TRIE_ARY \
	(2) // lower = slower + smaller, higher = faster + larger (to some extent, see
		// https://nrk.neocities.org/articles/hash-trees-and-tries)

struct HashTrieNode
{
	HashTrieNode *child[1 << HASH_TRIE_ARY];
	const char *key;
	void *value;
	HashTrieNode *next;
};

typedef struct
{
	HashTrieNode *head;
	HashTrieNode **tail;
} HashTrie;

static const char *hash_trie_dup_str_(Allocator *a, const char *str)
{
	size_t n = strlen(str);
	char *newstr = (char*)a->malloc(a->ctx, n + 1);
	memcpy(newstr, str, n);
	newstr[n] = 0;
	return newstr;
}

// https://github.com/skeeto/scratch/blob/master/misc/rainbow.c
// static uint64_t permute64(uint64_t x)
// {
//     x += 1111111111111111111u; x ^= x >> 32;
//     x *= 1111111111111111111u; x ^= x >> 32;
//     return x;
// }

static void hash_trie_init(HashTrie *trie)
{
	trie->head = NULL;
	trie->tail = &trie->head;
}

// requires Graphviz
// dot -Tpng trie.dot -o trie.png
// printf("digraph BinaryTree {\n");
// printf("\tnode [shape=circle];\n");

// for(HashTrieNode *it = ht.head; it; it = it->next)
// {
//     printf("\t%s [label=\"%s\"];\n", it->key, it->key);
// }
// for(HashTrieNode *it = ht.head; it; it = it->next)
// {
//     for(size_t k = 0; k < 4; ++k)
//     {
//         if(it->child[k])
//         {
//             HashTrieNode *child = it->child[k];
//             printf("\t%s -> %s;\n", it->key, child->key);
//         }
//     }
// }
// printf("}\n");

// After trying FNV, it seemed to perform worse

#include <ctype.h>

static uint64_t hash_trie_hash_func_(const char *s, bool case_sensitive)
{
	uint64_t h = 0x100;
	for(ptrdiff_t i = 0; s[i]; i++)
	{
		h ^= case_sensitive ? s[i] : tolower(s[i]);
		h *= 1111111111111111111u;
	}
	return h;
}

// If no arena is provided, it reverts to a lookup and returns null when the key is not found. It allows one function to
// flexibly serve both modes.

#ifndef _WIN32
	#include <strings.h>
	#define stricmp strcasecmp
#endif

static HashTrieNode *hash_trie_upsert(HashTrie *trie, const char *key, /*void *initial_value,*/ Allocator *a, bool case_sensitive)
{
	HashTrieNode **m = &trie->head;
	int (*cmp)(const char *, const char *);
	cmp = case_sensitive ? strcmp : stricmp;
	for(uint64_t h = hash_trie_hash_func_(key, case_sensitive);; h <<= HASH_TRIE_ARY)
	{
		if(!*m)
		{
			if(!a)
			{
				return NULL;
			}
            
            if(!trie->tail)
            {
                hash_trie_init(trie);
            }

			// HashTrieNode *new_node = new(a, HashTrieNode, 1);
			HashTrieNode *new_node = (HashTrieNode*)a->malloc(a->ctx, sizeof(HashTrieNode));
			memset(new_node, 0, sizeof(HashTrieNode));
			// memset(new_node, 0, sizeof(HashTrieNode));
			new_node->key = hash_trie_dup_str_(a, key);
			// new_node->next = 0;
			// new_node->value = initial_value;

			*m = new_node;
			*trie->tail = new_node;
			trie->tail = &new_node->next;
			return new_node;
		}
		if(!cmp((*m)->key, key))
		{
			return *m;
		}
		m = &(*m)->child[h >> (64 - HASH_TRIE_ARY)];
	}
	return NULL;
}
