/*
 * Copyright (c) 2024, Manolache Maria-Catalina 313CA
 */

#include "load_balancer.h"

#include "list_queue_hashtable_functions.h"
#include "server.h"

/* functie care adauga, in ordine, o eticheta in vectorul de etichete
*/
void add_tag_in_order(int *tag_array, int tag, int array_size) {
	// se calculeaza hashul etichetei de inserat
	unsigned int tag_hash = hash_uint(&tag);
	// se obtine pozitia pe care trebuie sa fie inserata eticheta, cautand
	// ultima eticheta cu hashul mai mic decat cel al etichetei de inserat
	int i = 0;
	for (i = 0; i < array_size - 1; i++) {
		if (hash_uint(&tag_array[i]) < tag_hash) {
			continue;
		} else if (hash_uint(&tag_array[i]) == tag_hash) {
			// cautam ultima eticheta mai mica decat nr de adaugat,
			// dar cu acelasi hash
			while (hash_uint(&tag_array[i]) == tag_hash && tag > tag_array[i])
				i++;
		} else {
			// s-a gasit pozitia
			break;
		}
	}
	// deplasarea elementelor vectorului pentru a face loc pentru noua eticheta
	for (int j = array_size - 1; j > i; j--)
		tag_array[j] = tag_array[j - 1];
	tag_array[i] = tag;
}

/* functie care elimina o eticheta din vectorul de etichete
*/
void remove_tag(int *tag_array, int tag, int array_size) {
	// se calculeaza hashul etichetei de eliminat
	unsigned int tag_hash = hash_uint(&tag);
	// se cauta pozitia etichetei in vectorul de etichete
	int i = 0;
	for (i = 0; i < array_size; i++)
		if (hash_uint(&tag_array[i]) == tag_hash)
			break;

	// se sterge elementul de pe pozitia i
	for (int j = i; j < array_size - 1; j++)
		tag_array[j] = tag_array[j + 1];
}

/* functie care gaseste serverul "vecin" al altui server, pe hash ring
* serverele sunt reprezentate prin etichete, in vectorul de etichete
*/
int find_neighbour_server(int *tag_array, int tag, int array_size) {
	// se calculeaza hashul etichetei
	unsigned int tag_hash = hash_uint(&tag);
	// se cauta urmatoarea eticheta cu hashul mai mare decat cel al etichetei
	// date ca parametru
	for (int i = 0; i < array_size; i++)
		if (hash_uint(&tag_array[i]) > tag_hash)
			return tag_array[i];

	// daca hashul serv dat este mai mare decat hashurile tuturor etichetelor,
	// vecinul lui va fi primul server
	return tag_array[0];
}

/* functie care verifica daca un document trebuie mutat
* de pe un server pe altul
*/
bool is_doc_moved(int server_id, int last_server, char *doc) {
	// daca documentul trebuie mutat pe primul server, hashul lui va fi mai
	// mare decat hashul ultimului server in vectorul de etichete
	if (hash_uint(&last_server) < hash_string(doc) &&
		hash_string(doc) > hash_uint(&server_id))
		return true;
	// documentul va fi mutat daca hashul serverului pe care se va muta este
	// mai mare decat hashul documentului
	return hash_uint(&server_id) > hash_string(doc);
}

/* functie care executa taskurile ramase in coada de requesturi a unui server
*/
void handle_remaining_requests(server *s) {
	// se da serverului un request gol, care sa initieze executia taskurilor
	request *foo = malloc(sizeof(request));
	DIE(foo == NULL, "Failed to allocate memory.\n");

	foo->doc_content = malloc(DOC_CONTENT_LENGTH * sizeof(char));
	DIE(foo->doc_content == NULL, "Failed to allocate memory.\n");

	foo->doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
	DIE(foo->doc_name == NULL, "Failed to allocate memory.\n");

	// se initializeaza campurile requestului, pentru a nu
	// schimba in mod eronat ordinea din cache
	memset(foo->doc_name, 0, DOC_NAME_LENGTH);
	memset(foo->doc_content, 0, DOC_CONTENT_LENGTH);

	// se seteaza tipul requestului la "GET", pentru a pune in executie
	// seria de dequeue-uri si procesari ale requesturilor
	foo->type = 1;

	// se apeleaza functia de procesare a requesturilor
	response *resp = server_handle_request(s, foo);
	// se elibereaza memoria alocata pentru request si response, deoarece
	// rezultatul nu ne intereseaza, doar executia requesturilor
	free(foo->doc_name);
	free(foo->doc_content);
	free(foo);

	free(resp->server_log);
	if (resp->server_response)
		free(resp->server_response);
	free(resp);
}

/* functia gaseste un server spre care sa fie redirectionat un request
*/
int find_server_to_forward(int *tag_array, int array_size, char *doc) {
	// se calculeaza hashul documentului
	unsigned int doc_hash = hash_string(doc);
	// serverul spre care va fi redirectionat requestul va fi primul server
	// cu hashul mai mare decat hashul documentului
	for (int i = 0; i < array_size; i++)
		if (hash_uint(&tag_array[i]) > doc_hash)
			return tag_array[i];
	// daca hashul doc. este mai mare decat hashurile tuturor etichetelor,
	// serverul spre care va fi redirectionat requestul va fi primul server
	return tag_array[0];
}

/* functie care creeaza un load balancer
*/
load_balancer *init_load_balancer(bool enable_vnodes) {
	// se aloca memorie pentru load balancer
	load_balancer *main = malloc(sizeof(load_balancer));
	DIE(main == NULL, "Failed to allocate memory.\n");
	// se initializeaza campurile
	main->hash_function_servers = hash_uint;
	main->hash_function_docs = hash_string;
	main->enable_vnodes = enable_vnodes;
	// se aloca memorie pentru vectorul care face legatura intre eticheta si
	// indexul serverului
	main->tag_to_index = calloc(MAX_SERVERS, sizeof(int));
	main->servers = NULL;
	main->s_tags = NULL;
	main->nr_servers = 0;
	return main;
}

/* functie care adauga un server in load balancer
*/
void loader_add_server(load_balancer *main, int server_id, int cache_size) {
	// se verifica daca numarul serverelor nu depaseste maximul posibil
	if (main->nr_servers < MAX_SERVERS) {
		// se adauga eticheta serverului in vectorul de etichete
		main->tag_to_index[server_id] = main->nr_servers;
		// se obtine indexul serverului in vectorul de servere
		int curr_idx = main->tag_to_index[server_id];

		// se aloca memorie pentru serverul ce urmeaza sa fie adaugat si se
		// realoca vectorii de servere si etichete
		main->nr_servers++;
		main->servers =
			realloc(main->servers, main->nr_servers * sizeof(server *));
		main->servers[curr_idx] = init_server(cache_size);
		main->servers[curr_idx]->id = server_id;
		main->s_tags = realloc(main->s_tags, main->nr_servers * sizeof(int));

		// se adauga eticheta in ordine in vectorul de etichete
		add_tag_in_order(main->s_tags, server_id, main->nr_servers);
		// se cauta vecinul noului server si indexul lui in vectorul de servere
		int neighbour =
			find_neighbour_server(main->s_tags, server_id, main->nr_servers);
		int neighbour_idx = main->tag_to_index[neighbour];

		// se executa toate taskurile din coada serverului vecin
		handle_remaining_requests(main->servers[neighbour_idx]);

		// se distribuie documentele vecinului serverului nou, iar cele care
		// trebuie sa fie mutate sunt eliminate din cache si mutate
		dll_node_t *aux = main->servers[neighbour_idx]->database->head;
		while (aux != NULL) {
			// verificam daca documentul trebuie mutat pe noul server
			dll_node_t *next = aux->next;
			if (is_doc_moved(server_id, main->s_tags[main->nr_servers - 1],
							 ((doc_t *)aux->data)->doc_name)) {
				// eliminarea documentului din cache-ul vecinului
				lru_cache_remove(main->servers[neighbour_idx]->cache,
								 ((doc_t *)aux->data)->doc_name);

				// se face o copie a documentului
				doc_t *to_move = malloc(sizeof(doc_t));
				DIE(to_move == NULL, "Failed to allocate memory.\n");

				to_move->doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
				DIE(to_move->doc_name == NULL, "Failed to allocate memory.\n");

				snprintf(to_move->doc_name, DOC_NAME_LENGTH, "%s",
						 (char *)((doc_t *)aux->data)->doc_name);

				if (((doc_t *)aux->data)->doc_content) {
					to_move->doc_content =
						malloc(DOC_CONTENT_LENGTH * sizeof(char));
					DIE(to_move->doc_content == NULL,
						"Failed to allocate memory\n");
					snprintf(to_move->doc_content, DOC_CONTENT_LENGTH, "%s",
						   (char *)((doc_t *)aux->data)->doc_content);
				}

				// se adauga in database-ul noului server si se sterge din
				// database-ul vecinului
				dll_add_nth_node(main->servers[curr_idx]->database,
								 main->servers[curr_idx]->database->size,
								 to_move);
				dll_remove_node(main->servers[neighbour_idx]->database, aux);

				// se elibereaza memoria pentru copie
				free(((doc_t *)aux->data)->doc_name);
				if (((doc_t *)aux->data)->doc_content)
					free(((doc_t *)aux->data)->doc_content);
				free(aux->data);
				free(aux);
				free(to_move);
			}
			// se trece la urmatorul document din database-ul vecinului
			aux = next;
		}
	}
}

/* functie care elimina un server din load balancer
*/
void loader_remove_server(load_balancer *main, int server_id) {
	// se verifica daca numarul serverelor nu depaseste maximul posibil
	if (main->nr_servers < MAX_SERVERS) {
		// se obtine indexul serverului in vectorul de servere
		int curr_idx = main->tag_to_index[server_id];
		// se cauta vecinul serverului ce urmeaza sa fie eliminat si indexul
		// lui in vectorul de servere
		int neighbour =
			find_neighbour_server(main->s_tags, server_id, main->nr_servers);
		int neighbour_idx = main->tag_to_index[neighbour];

		// se executa toate taskurile din coada serverului ce urmeaza sa fie
		// eliminat
		handle_remaining_requests(main->servers[curr_idx]);

		// se distribuie documentele serverului de eliminat vecinului, iar cele
		// care trebuie sa fie mutate sunt eliminate din cache si mutate
		dll_node_t *aux = main->servers[curr_idx]->database->head;
		while (aux != NULL) {
			// eliminarea documentului din cache-ul serverului de eliminat
			dll_node_t *next = aux->next;
			lru_cache_remove(main->servers[curr_idx]->cache,
							 ((doc_t *)aux->data)->doc_name);

			// se face o copie a documentului
			doc_t *to_move = malloc(sizeof(doc_t));
			DIE(to_move == NULL, "Failed to allocate memory.\n");

			to_move->doc_name = malloc(DOC_NAME_LENGTH * sizeof(char));
			DIE(to_move->doc_name == NULL, "Failed to allocate memory.\n");

			snprintf(to_move->doc_name, DOC_NAME_LENGTH, "%s",
					 (char *)((doc_t *)aux->data)->doc_name);

			if (((doc_t *)aux->data)->doc_content) {
				to_move->doc_content =
					malloc(DOC_CONTENT_LENGTH * sizeof(char));
				DIE(to_move->doc_content == NULL,
					"Failed to allocate memory.\n");
				snprintf(to_move->doc_content, DOC_CONTENT_LENGTH, "%s",
						 (char *)((doc_t *)aux->data)->doc_content);
			}

			// se adauga in database-ul vecinului si se sterge din database-ul
			// serverului de eliminat
			dll_add_nth_node(main->servers[neighbour_idx]->database,
							 main->servers[neighbour_idx]->database->size,
							 to_move);
			dll_remove_node(main->servers[curr_idx]->database, aux);

			// se elibereaza memoria pentru copie
			free(((doc_t *)aux->data)->doc_name);
			if (((doc_t *)aux->data)->doc_content)
				free(((doc_t *)aux->data)->doc_content);
			free(aux->data);
			free(aux);
			free(to_move);
			// se trece la urmatorul document din database-ul serverului de
			// eliminat
			aux = next;
		}

		// se elimina eticheta serverului din vectorul de etichete
		remove_tag(main->s_tags, server_id, main->nr_servers);
		// se realoca memorie pentru vectorul de etichete
		main->nr_servers--;
		main->s_tags = realloc(main->s_tags, main->nr_servers * sizeof(int));

		// se scade indexul elementelor de dupa serverul eliminat
		for (int i = 0; i < main->nr_servers; i++)
			if (main->tag_to_index[main->s_tags[i]] > curr_idx)
				main->tag_to_index[main->s_tags[i]]--;

		// se elimina serverul din vectorul de servere si se shifteaza
		// elementele de dupa serverul eliminat
		free_server(&main->servers[curr_idx]);
		for (int i = curr_idx; i < main->nr_servers; i++) {
			main->servers[i] = main->servers[i + 1];
		}
		// se realoca memorie pentru vectorul de servere
		main->servers =
			realloc(main->servers, main->nr_servers * sizeof(server *));
		// se seteaza indexul etichetei serverului eliminat pe 0
		main->tag_to_index[server_id] = 0;
	}
}

/* functie care redirectioneaza un request catre un server
*/
response *loader_forward_request(load_balancer *main, request *req) {
	int server_id =
		find_server_to_forward(main->s_tags, main->nr_servers, req->doc_name);
	// se redirectioneaza requestul catre serverul gasit
	int server_idx = main->tag_to_index[server_id];
	response *resp = server_handle_request(main->servers[server_idx], req);
	return resp;
}

/* functie care elibereaza memoria alocata pentru un load balancer
*/
void free_load_balancer(load_balancer **main) {
	// se elibereaza memoria pentru toate serverele
	for (int i = 0; i < (*main)->nr_servers; i++)
		free_server(&(*main)->servers[i]);

	// se elibereaza memoria vectorilor si a structurii principale
	free((*main)->tag_to_index);
	free((*main)->s_tags);
	free((*main)->servers);
	free(*main);

	*main = NULL;
}
