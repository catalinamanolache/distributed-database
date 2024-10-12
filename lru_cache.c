/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#include "lru_cache.h"

#include <stdio.h>
#include <string.h>

#include "list_queue_hashtable_functions.h"
#include "utils.h"

/* functie care initializeaza lru cache, precum si toate campurile lui
*/
lru_cache *init_lru_cache(unsigned int cache_capacity) {
	lru_cache *cache = malloc(sizeof(lru_cache));
	DIE(cache == NULL, "Failed to allocate memory\n");

	cache->lru_ht = ht_create(cache_capacity, hash_string, compare_function,
							  key_val_free_function);
	cache->lru_dll = dll_create(sizeof(info));
	return cache;
}

/* functie care verifica daca cache-ul este plin
*/
bool lru_cache_is_full(lru_cache *cache) {
	return cache->lru_ht->size == cache->lru_ht->hmax;
}
/* functie care elibereaza memoria alocata pentru cache si toate campurile lui
*/
void free_lru_cache(lru_cache **cache) {
	// se elibereaza memoria alocata pentru toate nodurile din lista
	dll_node_t *node = (*cache)->lru_dll->head;
	while (node != NULL) {
		dll_node_t *aux = node;
		node = node->next;
		// se elibereaza campurile din nod
		key_val_free_function(aux->data);
		free(aux->data);
		free(aux);
		aux = NULL;
	}
	// se elibereaza memoria alocata pentru lista dublu inlantuita
	free((*cache)->lru_dll);

	// se elibereaza memoria alocata pentru hashtable
	ht_free((*cache)->lru_ht);

	// se elibereaza memoria alocata pentru cache
	free(*cache);
}

/* functie care adauga un nou element in cache, verificand daca cheia exista
* deja in cache
* daca cache-ul este plin, elimina cel mai vechi accesat element
*/
bool lru_cache_put(lru_cache *cache, void *key, void *value,
				   void **evicted_key) {
	// se calculeaza hash-ul cheii
	unsigned int hash = cache->lru_ht->hash_function(key);
	hash %= cache->lru_ht->hmax;

	if (ht_has_key(cache->lru_ht, key)) {
		// daca cheia exista deja in cache, se actualizeaza continutul si
		// ordinea in lista de recente
		// se cauta cheia in hashtable
		dll_node_t *aux = cache->lru_ht->buckets[hash]->head;
		while (aux != NULL) {
			if (cache->lru_ht->compare_function(((info *)aux->data)->key,
												key) == 0) {
				// actualizez lista de recente
				dll_node_t *to_modify = ((info *)aux->data)->value;
				// modificarea "valorii" din ht consta in modificarea valorii
				// nodului din lista de recente, adica eliminarea lui si
				// adaugarea lui la final
				dll_remove_node(cache->lru_dll, to_modify);
				dll_add_nth_node(cache->lru_dll, cache->lru_dll->size,
								 to_modify->data);
				// cheia exista deja in cache, returnam false
				return false;
			}
			aux = aux->next;
		}
	}
	// cheia nu exista in ht, se creaza o intrare noua in ht si in lista

	// verific daca cache e plin
	if (lru_cache_is_full(cache)) {
		// elimin cel mai vechi accesat element
		*evicted_key = (char *)malloc(DOC_NAME_LENGTH * sizeof(char));
		DIE(*evicted_key == NULL, "Failed to allocate memory\n");

		snprintf((char *)*evicted_key, DOC_NAME_LENGTH, "%s",
				 (char *)(((info *)cache->lru_dll->head->data)->key));
		lru_cache_remove(cache, *evicted_key);
	}

	// adaugare nou element in cache, in lista de recente
	info *new_node = malloc(sizeof(info));
	DIE(new_node == NULL, "Failed to allocate memory\n");

	new_node->key = malloc(DOC_NAME_LENGTH * sizeof(char));
	DIE(new_node->key == NULL, "Failed to allocate memory\n");
	memcpy(new_node->key, key, DOC_NAME_LENGTH);

	new_node->value = malloc(DOC_CONTENT_LENGTH * sizeof(char));
	DIE(new_node->value == NULL, "Failed to allocate memory\n");
	memcpy(new_node->value, value, DOC_CONTENT_LENGTH);

	// se adauga la final, fiind cel mai recent accesat element
	dll_add_nth_node(cache->lru_dll, cache->lru_dll->size, new_node);
	free(new_node);

	// adaugare nou element in cache, in hashtable
	info *new_node_ht = malloc(sizeof(info));
	DIE(new_node_ht == NULL, "Failed to allocate memory\n");

	new_node_ht->key = malloc(DOC_NAME_LENGTH * sizeof(char));
	DIE(new_node_ht->key == NULL, "Failed to allocate memory\n");
	memcpy(new_node_ht->key, key, DOC_NAME_LENGTH);

	// in ht valoarea este un pointer catre nodul cel mai recent din lista,
	// adaugat mai sus
	new_node_ht->value = cache->lru_dll->tail;
	// se adauga in lista de valori a bucket-ului corespunzator
	dll_add_nth_node(cache->lru_ht->buckets[hash],
					 cache->lru_ht->buckets[hash]->size, new_node_ht);

	free(new_node_ht);
	// se actualizeaza numarul de noduri din hashtable
	cache->lru_ht->size++;
	// cheia nu exista in cache, returnam true
	return true;
}

/* functie care returneaza valoarea asociata unei chei, daca aceasta exista in
* cache
*/
void *lru_cache_get(lru_cache *cache, void *key) {
	// se calculeaza hash-ul cheii
	unsigned int hash = cache->lru_ht->hash_function(key);
	hash %= cache->lru_ht->hmax;

	dll_node_t *aux = cache->lru_ht->buckets[hash]->head;
	while (aux != NULL) {
		if (cache->lru_ht->compare_function(((info *)aux->data)->key, key) ==
			0) {
			// daca cheia exista in cache, se actualizeaza ordinea in lista de
			// recente
			dll_node_t *to_move = ((info *)aux->data)->value;
			dll_remove_node(cache->lru_dll, to_move);
			dll_add_nth_node(cache->lru_dll, cache->lru_dll->size,
							 to_move->data);
			free(to_move->data);
			free(to_move);
			// se actualizeaza "valoarea" din hashtable, adica pointerul catre
			// nodul din lista de recente
			((info *)aux->data)->value = cache->lru_dll->tail;
			// se returneaza valoarea asociata cheii
			return ((info *)aux->data)->value;
		}
		aux = aux->next;
	}
	// se returneaza NULL daca cheia nu exista in cache
	return NULL;
}

/* functie care elimina un element din cache
*/
void lru_cache_remove(lru_cache *cache, void *key) {
	// se calculeaza hash-ul cheii
	unsigned int hash = cache->lru_ht->hash_function(key);
	hash %= cache->lru_ht->hmax;

	dll_node_t *aux = cache->lru_ht->buckets[hash]->head;
	while (aux != NULL) {
		if (cache->lru_ht->compare_function(((info *)aux->data)->key, key) ==
			0) {
			// se elimina elementul din lista de recente si se elibereaza
			// memoria alocata
			dll_node_t *to_remove = ((info *)aux->data)->value;
			dll_remove_node(cache->lru_dll, to_remove);
			cache->lru_ht->key_val_free_function(to_remove->data);
			free(to_remove->data);
			free(to_remove);

			// se elimina elementul din hashtable si se elibereaza memoria
			// alocata
			dll_remove_node(cache->lru_ht->buckets[hash], aux);
			if (((info *)aux->data)->key)
				free(((info *)aux->data)->key);
			free(aux->data);
			free(aux);
			// se actualizeaza numarul de noduri din hashtable
			cache->lru_ht->size--;
			return;
		}
		aux = aux->next;
	}
}
