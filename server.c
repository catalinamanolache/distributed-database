/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#include "server.h"

#include <stdio.h>

#include "list_queue_hashtable_functions.h"
#include "lru_cache.h"
#include "utils.h"

/* functie care se ocupa de requesturile de tipul EDIT si returneaza
 * response-ul corespunzator
*/
static response *server_edit_document(server *s, char *doc_name,
									  char *doc_content) {
	// se aloca memorie pentru response
	response *resp = malloc(sizeof(response));
	DIE(resp == NULL, "Failed to allocate memory\n");

	resp->server_log = malloc(MAX_LOG_LENGTH * sizeof(char));
	DIE(resp->server_log == NULL, "Failed to allocate memory\n");

	resp->server_response = malloc(MAX_RESPONSE_LENGTH * sizeof(char));
	DIE(resp->server_response == NULL, "Failed to allocate memory\n");

	resp->server_id = s->id;
	// se cauta daca documentul este in cache
	dll_node_t *ptr_to_content = lru_cache_get(s->cache, doc_name);
	if (ptr_to_content) {
		// daca documentul este in cache, se modifica continutul acestuia
		// si se actualizeaza log-ul si respunsul
		snprintf(resp->server_response, MAX_RESPONSE_LENGTH, MSG_B, doc_name);
		snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_HIT, doc_name);

		// se modifica continutul documentului din cache, din lista de recente
		snprintf(((info *)ptr_to_content->data)->value, DOC_CONTENT_LENGTH,
				 "%s", doc_content);

		// se modifica continutul documentului din database
		dll_node_t *aux = s->database->head;
		for (unsigned int i = 0; i < s->database->size; i++) {
			if (!strcmp((char *)((doc_t *)aux->data)->doc_name, doc_name)) {
				snprintf((char *)((doc_t *)aux->data)->doc_content, DOC_CONTENT_LENGTH,
						 "%s", doc_content);
				break;
			}
			aux = aux->next;
		}
	} else {
		// documentul nu e in cache, se cauta in database
		unsigned int i = 0;
		dll_node_t *aux = s->database->head;
		for (i = 0; i < s->database->size; i++) {
			if (!strcmp((char *)((doc_t *)aux->data)->doc_name, doc_name)) {
				// s-a gasit documentul in database
				break;
			}
			aux = aux->next;
		}

		if (i == s->database->size || aux == NULL) {
			// documentul nu e in database, se adauga in cache si in database
			void *evicted_key = NULL;
			char *doc_copy = strdup(doc_name);
			// se adauga in cache
			lru_cache_put(s->cache, doc_name, doc_content, &evicted_key);
			if (evicted_key) {
				// cache a fost plin si s-a eliminat o cheie
				// se actualizeaza log-ul
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_EVICT, doc_copy,
						(char *)evicted_key);
				free(evicted_key);
			} else {
				// cache nu a fost plin
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_MISS, doc_name);
			}
			free(doc_copy);

			// se aloca memorie pentru o noua intrare in database
			doc_t *new_node = malloc(sizeof(doc_t));
			DIE(new_node == NULL, "Failed to allocate memory\n");

			new_node->doc_content = malloc(DOC_CONTENT_LENGTH * sizeof(char));
			DIE(new_node->doc_content == NULL, "Failed to allocate memory\n");
			snprintf((char *)new_node->doc_content, DOC_CONTENT_LENGTH, "%s",
					 doc_content);

			new_node->doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
			DIE(new_node->doc_name == NULL, "Failed to allocate memory\n");
			snprintf((char *)new_node->doc_name, DOC_NAME_LENGTH, "%s",
					 doc_name);

			// se adauga in database si se actualizeaza response-ul
			dll_add_nth_node(s->database, s->database->size, new_node);
			free(new_node);
			snprintf(resp->server_response, MAX_RESPONSE_LENGTH, MSG_C,
					 doc_name);
		} else {
			// documentul este in database, se adauga in cache si se
			// actualizeaza continutul acestuia
			void *evicted_key = NULL;
			// se adauga in cache
			lru_cache_put(s->cache, doc_name, doc_content, &evicted_key);
			if (evicted_key) {
				// cache a fost plin si s-a eliminat o cheie
				// se actualizeaza log-ul
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_EVICT, doc_name,
						(char *)evicted_key);
				free(evicted_key);
			} else {
				// cache nu a fost plin
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_MISS, doc_name);
			}

			// se modifica continutul documentului din database
			snprintf((char *)((doc_t *)aux->data)->doc_content,
			 		 DOC_CONTENT_LENGTH, "%s", doc_content);
			// se actualizeaza response-ul
			snprintf(resp->server_response, MAX_RESPONSE_LENGTH, MSG_B,
					 doc_name);
		}
	}
	return resp;
}

/* functie care se ocupa de requesturile de tipul GET si returneaza
 * response-ul corespunzator
*/
static response *server_get_document(server *s, char *doc_name) {
	// se aloca memorie pentru response
	response *resp = malloc(sizeof(response));
	DIE(resp == NULL, "Failed to allocate memory\n");

	resp->server_log = malloc(MAX_LOG_LENGTH * sizeof(char));
	DIE(resp->server_log == NULL, "Failed to allocate memory\n");

	resp->server_id = s->id;

	// se cauta daca documentul este in cache
	dll_node_t *ptr_to_content = lru_cache_get(s->cache, doc_name);
	if (ptr_to_content) {
		// daca documentul este in cache, se actualizeaza log-ul si
		// response-ul este continutul documentului
		resp->server_response = malloc(MAX_RESPONSE_LENGTH * sizeof(char));
		DIE(resp->server_response == NULL, "Failed to allocate memory\n");
		snprintf(resp->server_response, MAX_RESPONSE_LENGTH, "%s",
			   (char *)((info *)ptr_to_content->data)->value);
		snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_HIT, doc_name);
	} else {
		// documentul nu e in cache, se cauta in database
		unsigned int i = 0;
		dll_node_t *aux = s->database->head;
		for (i = 0; i < s->database->size; i++) {
			if (!strcmp((char *)((doc_t *)aux->data)->doc_name, doc_name)) {
				// s-a gasit documentul in database
				break;
			}
			aux = aux->next;
		}

		if (i == s->database->size || aux == NULL) {
			// documentul nu e in database, se actualizeaza log-ul si
			// response-ul este NULL
			resp->server_response = NULL;
			snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_FAULT, doc_name);
		} else {
			// documentul este in database, se adauga in cache si se returneaza
			// continutul acestuia
			resp->server_response = malloc(MAX_RESPONSE_LENGTH * sizeof(char));
			DIE(resp->server_response == NULL, "Failed to allocate memory\n");

			snprintf(resp->server_response, DOC_CONTENT_LENGTH, "%s",
					 (char *)((doc_t *)aux->data)->doc_content);

			// se adauga in cache
			void *evicted_key = NULL;
			lru_cache_put(s->cache, doc_name, resp->server_response,
						  &evicted_key);
			if (evicted_key) {
				// cache a fost plin si s-a eliminat o cheie
				// se actualizeaza log-ul
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_EVICT, doc_name,
						(char *)evicted_key);
				free(evicted_key);
			} else {
				// cache nu a fost plin
				snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_MISS, doc_name);
			}
		}
	}
	return resp;
}

/* functie care initializeaza un server cu un cache de dimensiune cache_size
 * si toate campurile acestuia
*/
server *init_server(unsigned int cache_size) {
	lru_cache *cache = init_lru_cache(cache_size);
	queue_t *task_queue = q_create(TASK_QUEUE_SIZE, sizeof(request));
	init_request_queue(task_queue);

	dll_t *database = dll_create(sizeof(doc_t));

	server *s = malloc(sizeof(server));
	DIE(s == NULL, "Failed to allocate memory\n");

	s->cache = cache;
	s->task_queue = task_queue;
	s->database = database;
	return s;
}
/* functie care se ocupa de requesturile primite de la client si returneaza
 * response-ul corespunzator
*/
response *server_handle_request(server *s, request *req) {
	char *type = get_request_type_str(req->type);
	response *resp;
	// se verifica tipul request-ului si se apeleaza functia corespunzatoare
	if (!strcmp(type, "EDIT")) {
		// se face o copie a requestului pentru a fi adaugat in coada
		request *req_copy = malloc(sizeof(request));
		DIE(req_copy == NULL, "Failed to allocate memory\n");

		req_copy->type = req->type;
		req_copy->doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
		DIE(req_copy->doc_name == NULL, "Failed to allocate memory\n");

		snprintf(req_copy->doc_name, DOC_NAME_LENGTH, "%s", req->doc_name);
		req_copy->doc_content = NULL;
		if (req->doc_content != NULL) {
			req_copy->doc_content = malloc(DOC_CONTENT_LENGTH * sizeof(char));
			DIE(req_copy->doc_content == NULL, "Failed to allocate memory\n");

			snprintf(req_copy->doc_content, DOC_CONTENT_LENGTH, "%s", req->doc_content);
		}

		// se adauga request-ul in coada
		q_enqueue(s->task_queue, req_copy);

		// se aloca memorie pentru response
		resp = malloc(sizeof(response));
		DIE(resp == NULL, "Failed to allocate memory\n");

		resp->server_log = malloc(MAX_LOG_LENGTH * sizeof(char));
		DIE(resp->server_log == NULL, "Failed to allocate memory\n");

		resp->server_response = malloc(MAX_RESPONSE_LENGTH * sizeof(char));
		DIE(resp->server_response == NULL, "Failed to allocate memory\n");

		resp->server_id = s->id;

		// se actualizeaza log-ul si response-ul
		snprintf(resp->server_log, MAX_LOG_LENGTH, LOG_LAZY_EXEC,
				 s->task_queue->size);
		snprintf(resp->server_response, MAX_RESPONSE_LENGTH, MSG_A,
				 type, req_copy->doc_name);

		free(req_copy);
	} else if (!strcmp(type, "GET")) {
		// se face o copie a numelui documentului pentru a fi folosit in
		// functia server_get_document
		char *doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
		DIE(doc_name == NULL, "Failed to allocate memory\n");

		snprintf(doc_name, DOC_NAME_LENGTH, "%s", req->doc_name);

		while (q_is_empty(s->task_queue) == 0) {
			// se executa toate request-urile de tipul EDIT din coada
			request *to_process = q_front(s->task_queue);
			response *resp_edit = server_edit_document(s, to_process->doc_name,
													   to_process->doc_content);
			// se printeaza raspunsul si se elibereaza memoria request-ului
			PRINT_RESPONSE(resp_edit);

			free(to_process->doc_name);
			if (to_process->doc_content)
				free(to_process->doc_content);

			// se trece la urmatorul request
			q_dequeue(s->task_queue);
		}
		// se executa request-ul de tipul GET
		resp = server_get_document(s, doc_name);
		free(doc_name);
	}
	return resp;
}

/* funcie care elibereaza memoria alocata pentru un server
 * si toate campurile acestuia
*/
void free_server(server **s) {
	// se elibereaza memoria alocata pentru cache, coada si database
	free_lru_cache(&((*s)->cache));

	q_free((*s)->task_queue);

	// se elibereaza memoria unei liste dublu inlantuite
	// si a documentelor din aceasta
	dll_node_t *node = ((*s)->database)->head;
	while (node != NULL) {
		dll_node_t *aux = node;
		node = node->next;
		doc_t_free_function(aux->data);
		free(aux->data);
		free(aux);
	}
	free(((*s)->database));
	free(*s);
}
