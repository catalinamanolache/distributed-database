/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#ifndef LIST_QUEUE_HASHTABLE_FUNCTIONS_H
#define LIST_QUEUE_HASHTABLE_FUNCTIONS_H

#include "structs.h"

dll_t *dll_create(unsigned int data_size);
void dll_add_nth_node(dll_t *list, unsigned int n, const void *data);
void dll_remove_node(dll_t *list, dll_node_t *node);
void dll_free(dll_t **pp_list);

void key_val_free_function(void *data);
void doc_t_free_function(void *data);

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *));
int ht_has_key(hashtable_t *ht, void *key);
void ht_free(hashtable_t *ht);
int compare_function(void *a, void *b);

queue_t *q_create(unsigned int max_size, unsigned int data_size);
void init_request_queue(queue_t *q);
void free_request_queue(queue_t *q);
unsigned int q_is_empty(queue_t *q);
void *q_front(queue_t *q);
int q_dequeue(queue_t *q);
int q_enqueue(queue_t *q, void *new_data);
void q_clear(queue_t *q);
void q_free(queue_t *q);

#endif /* LIST_QUEUE_HASHTABLE_FUNCTIONS_H */
