/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct dll_node_t {
	void *data;
	struct dll_node_t *next;
	struct dll_node_t *prev;
} dll_node_t;

typedef struct dll_t {
	dll_node_t *head;
	dll_node_t *tail;
	unsigned int size;
	unsigned int data_size;
} dll_t;

typedef struct queue_t {
	// dimensiunea maxima a cozii
	unsigned int max_size;
	// dimensiunea cozii
	unsigned int size;
	// dimensiunea in bytes a tipului de date stocat in coada
	unsigned int data_size;
	// indexul de la care se vor efectua operatiile de front si dequeue
	unsigned int read_idx;
	// indexul de la care se vor efectua operatiile de enqueue
	unsigned int write_idx;
	// bufferul ce stocheaza elementele cozii
	void **buff;
} queue_t;

typedef struct hashtable_t {
	// vector de liste dublu inlantuite
	dll_t **buckets;
	// nr. total de noduri existente curent in toate bucket-urile
	unsigned int size;
	// nr. de bucket-uri
	unsigned int hmax;
	// pointer la o functie pentru a calcula valoarea hash asociata cheilor
	unsigned int (*hash_function)(void *);
	// pointer la o functie pentru a compara doua chei
	int (*compare_function)(void *, void *);
	// pointer la o functie pentru a elibera memoria ocupata de cheie si
	// valoare
	void (*key_val_free_function)(void *);
} hashtable_t;

typedef struct info {
	void *key;
	void *value;
} info;

typedef struct doc_t {
	void *doc_name;
	void *doc_content;
} doc_t;

#endif /* STRUCTS_H */
