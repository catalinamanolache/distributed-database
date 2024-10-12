/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#include "list_queue_hashtable_functions.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "server.h"

/* functie care creeaza o lista dublu inlantuita
*/ 
dll_t *dll_create(unsigned int data_size) {
	dll_t *list = malloc(1 * sizeof(dll_t));
	DIE(list == NULL, "Failed to allocate memory\n");

	list->data_size = data_size;
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	return list;
}

/* functia care adauga un nod intr-o lista dublu inlantuita
 * pe o pozitie arbitrara data ca parametru
*/
void dll_add_nth_node(dll_t *list, unsigned int n, const void *data) {
	dll_node_t *new_node = malloc(1 * sizeof(dll_node_t));
	DIE(new_node == NULL, "Failed to allocate memory\n");
	new_node->data = malloc(list->data_size);
	DIE(new_node->data == NULL, "Failed to allocate memory\n");
	memcpy(new_node->data, data, list->data_size);

	if (list->head == NULL) {
		// lista este goala
		list->head = new_node;
		list->tail = new_node;
		new_node->next = NULL;
		new_node->prev = NULL;
		list->size++;
	} else if (n == 0) {
		// adaugare la inceput
		new_node->next = list->head;
		new_node->prev = NULL;
		list->head->prev = new_node;
		list->head = new_node;
		list->size++;
	} else if (n >= list->size) {
		// adaugare la final
		dll_node_t *aux = list->tail;
		aux->next = new_node;
		new_node->prev = aux;
		new_node->next = NULL;
		list->tail = new_node;
		list->size++;
	} else {
		// orice alta pozitie
		dll_node_t *aux = list->head;
		for (unsigned int i = 0; i < n - 1; i++)
			aux = aux->next;
		new_node->next = aux->next;
		aux->next->prev = new_node;
		aux->next = new_node;
		new_node->prev = aux;
		list->size++;
	}
}

/* functie care sterge un anumit nod, dat ca parametru, dintr-o
 * lista dublu inlantuita
*/
void dll_remove_node(dll_t *dll, dll_node_t *node) {
	if (node == dll->head) {
		// daca nodul este capul listei
		dll->head = node->next;
		// se repara legatura
		if (dll->head != NULL)
			dll->head->prev = NULL;
		dll->size--;
		return;
	} else if (node == dll->tail) {
		// daca nodul este coada listei
		dll->tail = node->prev;
		// se repara legatura
		if (dll->tail != NULL)
			dll->tail->next = NULL;
		dll->size--;
		return;
	} else {
		// orice alta pozitie
		node->prev->next = node->next;
		node->next->prev = node->prev;
		dll->size--;
	}
}

/* functie care elibereeza memoria unei perechi cheie-valoare
*/
void key_val_free_function(void *data) {
	if (((info *)data)->key)
		free(((info *)data)->key);
	if (((info *)data)->value)
		free(((info *)data)->value);
}

/* functie care elibereaza memoria unei variabile de tipul doc_t
*/
void doc_t_free_function(void *data) {
	if (((doc_t *)data)->doc_name)
		free(((doc_t *)data)->doc_name);
	if (((doc_t *)data)->doc_content)
		free(((doc_t *)data)->doc_content);
}

/* functie care creeaza un hashtable, cu maximum hmax buckets, ce va contine
 * pointeri la functii de hash, comparare de chei si eliberare de memorie
*/
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *)) {
	// se aloca memorie pentru hashtable si se initializeaza campurile
	hashtable_t *hashtable = malloc(1 * sizeof(hashtable_t));
	DIE(hashtable == NULL, "Failed to allocate memory\n");
	hashtable->buckets = malloc(hmax * sizeof(dll_t *));
	DIE(hashtable->buckets == NULL, "Failed to allocate memory\n");
	hashtable->hmax = hmax;
	hashtable->size = 0;
	hashtable->hash_function = hash_function;
	hashtable->compare_function = compare_function;
	hashtable->key_val_free_function = key_val_free_function;

	// se initialieaza fiecare bucket
	for (unsigned int i = 0; i < hmax; i++)
		hashtable->buckets[i] = dll_create(sizeof(info));
	return hashtable;
}

/* functie care verifica daca o cheie, data ca parametru, se afla in hashtable
* functia va returna 1 daca cheia este prezenta in hashtable si 0 in caz contrar
*/
int ht_has_key(hashtable_t *ht, void *key) {
	unsigned int hash = ht->hash_function(key);
	hash %= ht->hmax;

	dll_node_t *aux = ht->buckets[hash]->head;

	while (aux != NULL) {
		if (ht->compare_function(((info *)(aux->data))->key, key) == 0)
			return 1;
		aux = aux->next;
	}
	return 0;
}

/* functie care elibereaza memoria pentru in hashtable
*/
void ht_free(hashtable_t *ht) {
	for (unsigned int i = 0; i < ht->hmax; i++) {
		dll_node_t *node = ht->buckets[i]->head;
		while (node != NULL) {
			dll_node_t *aux = node;
			node = node->next;
			// se elibereaza doar cheia deoarece pointerul catre valoare
			// a fost eliberat odata cu lista de acces recent
			free(((info *)aux->data)->key);
			free(aux->data);
			free(aux);
		}
		free(ht->buckets[i]);
	}

	free(ht->buckets);
	free(ht);
}

/* functie care compara doua chei sub forma de stringuri
 * functia va returna 0 daca cele doua stringuri sunt egale
*/
int compare_function(void *a, void *b) {
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/* functie care initializeaza coada de requesturi
*/
void init_request_queue(queue_t *q) {
	// initializez fiecare camp de tipul request cu NULL
	for (unsigned int i = 0; i < q->max_size; i++) {
		((request *)q->buff[i])->doc_name = NULL;
		((request *)q->buff[i])->doc_content = NULL;
	}
}

/* functie care creeaza coada de task-uri, avand dimensiunea maxima
 * TASK_QUEUE_SIZE
*/
queue_t *q_create(unsigned int max_size, unsigned int data_size) {
	queue_t *q = malloc(1 * sizeof(queue_t));
	DIE(q == NULL, "Failed to allocate memory\n");
	q->buff = malloc(max_size * sizeof(void *));
	DIE(q->buff == NULL, "Failed to allocate memory\n");
	for (unsigned int i = 0; i < max_size; i++) {
		q->buff[i] = malloc(data_size);
		DIE(q->buff[i] == NULL, "Failed to allocate memory\n");
	}
	q->data_size = data_size;
	q->max_size = max_size;
	q->size = 0;
	q->read_idx = 0;
	q->write_idx = 0;
	return q;
}

/* functia verifica daca o coada este goala
 * functia va returna 1 daca coada este goala si 0 in caz contrar
*/
unsigned int q_is_empty(queue_t *q) {
	if (q->size == 0)
		return 1;
	return 0;
}

/* functia returneaza primul element din coada
*/
void *q_front(queue_t *q) {
	return q->buff[q->read_idx];
}

/* functia elimina primul element din coada
 * functia va returna 1 daca s-a eliminat un element si 0 in caz contrar
*/
int q_dequeue(queue_t *q) {
	if (q->size) {
		if (q->buff[q->read_idx]) {
			free(q->buff[q->read_idx]);
			q->buff[q->read_idx] = NULL;
			q->read_idx = (q->read_idx + 1) % q->max_size;
			q->size--;
			return 1;
		}
	}
	return 0;
}

/* functia adauga un element in coada
 * functia va returna 1 daca s-a adaugat un element si 0 in caz contrar
*/
int q_enqueue(queue_t *q, void *new_data) {
	if (q->size <= q->max_size) {
		if (q->buff[q->write_idx] == NULL) {
			q->buff[q->write_idx] = malloc(q->data_size);
			DIE(q->buff[q->write_idx] == NULL, "Failed to allocate memory\n");
		}
		memcpy(q->buff[q->write_idx], new_data, q->data_size);
		q->write_idx = (q->write_idx + 1) % q->max_size;
		q->size++;
		return 1;
	}
	return 0;
}

/* functia elibereaza campurile pentru o coada de requesturi
*/
void free_request_queue(queue_t *q) {
	for (unsigned int i = 0; i < q->max_size; i++) {
		if (q->buff[i] && ((request *)q->buff[i])->doc_name &&
			((request *)q->buff[i])->doc_content) {
			free(((request *)q->buff[i])->doc_name);
			if (((request *)q->buff[i])->doc_content)
				free(((request *)q->buff[i])->doc_content);
		}
	}
}

/* functia elibereaza memoria alocata pentru toate elementele din coada
*/
void q_clear(queue_t *q) {
	// eliberez campurile requesturilor
	free_request_queue(q);

	for (unsigned int i = 0; i < q->max_size; i++) {
		// verific daca in fiecare element din coada, exista un request
		free(q->buff[i]);
	}
	q->size = 0;
	q->read_idx = 0;
	q->write_idx = 0;
}

/* functia elibereaza memoria alocata pentru toate elementele cozii
 * si pentru coada in sine
*/
void q_free(queue_t *q) {
	q_clear(q);
	free(q->buff);
	free(q);
}
