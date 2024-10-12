**Nume:** Manolache Maria-Catalina
**Grupa:** 313CAb

## Tema 2 - Distributed Database

# Fisiere aditionale
- `structs.h`: Contine toate structurile de date (auxiliare) folosite in
   implementare.
- `list_queue_hashtable_functions.c`: Contine toate functiile elementare
   necesare pentru lucrul cu liste dublu inlantuite, cozi si hashtable,
   precum functii pentru eliberarea memoriei, crearea structurilor de date,
   etc.
- `list_queue_hashtable_functions.h`: Header-ul fisierului anterior.

# In continuare voi explica fiecare functie din fisierele de implementat.
## LRU CACHE

- `init_lru_cache`: Initializeaza cache-ul cu o capacitate data ca parametru.
  Aloca memorie pentru structura cache si initializeaza hashtable-ul si lista
  dublu inlantuita.

- `lru_cache_is_full`: Verifica daca cache-ul LRU este plin. Returneaza true
  daca dimensiunea hashtable-ului este egala cu capacitatea maxima a acestuia,
  adica daca cache-ul este plin.

- `free_lru_cache`: Elibereaza memoria alocata pentru cache si toate campurile
  lui. Parcurge lista dublu inlantuita si hashtable-ul, eliberand memoria
  alocata pentru fiecare nod, dar si structurile in sine.

- `lru_cache_put`: Adauga un nou element in cache. Daca cheia exista deja in
  cache, actualizeaza valoarea si ordinea in lista de elemente recente. In caz
  contrar, adauga un nou element la finalul listei de elemente recente si in
  hashtable. Daca cache-ul este plin, elimina elementul cel mai putin recent
  utilizat din cache. Functia returneaza true daca cheia nu exista in cache si
  false in caz contrar.

- `lru_cache_get`: Returneaza valoarea asociata cu o cheie, daca aceasta exista
  in cache. De asemenea, actualizeaza ordinea in lista de elemente recente.

- `lru_cache_remove`: Elimina un element din cache. Parcurge hashtable-ul pentru
  a gasi cheia, apoi elimina nodul corespunzator din lista dublu inlantuita si
  hashtable.

## LOAD BALANCER

- `add_tag_in_order`: Adauga o eticheta in vectorul de etichete, pastrand
  ordinea crescatoare a vectorului. Calculeaza hash-ul etichetei de inserat si
  gaseste pozitia pe care trebuie sa fie inserata eticheta, apoi deplaseaza
  elementele vectorului pentru a face loc pentru noua eticheta.

- `remove_tag`: Elimina o eticheta din vectorul de etichete. Calculeaza hash-ul
  etichetei de eliminat si gaseste pozitia etichetei in vectorul de etichete,
  apoi sterge elementul de pe acea pozitie.

- `find_neighbour_server`: Gaseste serverul "vecin" al unui alt server pe hash
  ring. Calculeaza hash-ul etichetei si cauta urmatoarea eticheta cu hash-ul mai
  mare decat cel al etichetei date ca parametru pentru a o returna. Daca
  hash-ul serverului actual este cel mai mare din vectorul de etichete, vecinul
  sau este primul server.

- `is_doc_moved`: Verifica daca un document trebuie mutat de pe un server pe
  altul. Compara hash-urile documentului, serverului curent si ultimului server
  pentru a determina daca documentul trebuie mutat. Documentul va fi mutat daca
  hash-ul serverului este mai mare decat hash-ul documentului. Daca hash-ul 
  ultimului server este mai mic decat hash-ul documentului si hash-ul
  documentului este mai mare decat hash-ul serverului, atunci documentul trebuie
  mutat pe primul server (caz particular).

- `handle_remaining_requests`: Executa taskurile ramase in coada de requesturi a
  unui server. Creeaza un request gol si il trimite serverului pentru a initia
  executia taskurilor si ignora raspunsul la acest request gol.

- `find_server_to_forward`: Gaseste un server spre care sa fie redirectionat un
  request. Calculeaza hash-ul documentului si gaseste primul server cu hash-ul mai
  mare decat hash-ul documentului pentru redirectionare.

- `init_load_balancer`: Initializeaza un Load Balancer. Aloca memorie pentru
  structura Load Balancer si initializeaza campurile acesteia, inclusiv functiile
  de hash pentru servere si documente, si un vector care face legatura intre
  eticheta si indexul serverului.

- `loader_add_server`: Adauga un server in Load Balancer. Verifica daca numarul
  serverelor nu depaseste maximul posibil, adauga eticheta serverului in vectorul
  de etichete, aloca memorie pentru noul server si distribuie documentele 
  vecinului serverului nou, daca acestea trebuie sa fie distribuite (se verifica
  folosind functia `is_doc_moved`).

- `loader_remove_server`: Elimina un server din Load Balancer. Verifica daca
  numarul serverelor nu depaseste maximul posibil, obtine indexul serverului in
  vectorul de servere, executa toate taskurile din coada serverului ce urmeaza sa
  fie eliminat si distribuie documentele serverului de eliminat, vecinului.

- `loader_forward_request`: Redirectioneaza un request catre un server. Gaseste
  serverul spre care trebuie sa fie redirectionat requestul si apoi 
  redirectioneaza requestul catre acel server.

- `free_load_balancer`: Elibereaza memoria alocata pentru un Load Balancer.
  Elibereaza memoria pentru toate serverele, vectorii si structura principala a
  Load Balancer-ului.

## SERVER

- `server_edit_document`: Se ocupa de requesturile de tipul EDIT si returneaza
  response-ul corespunzator. In primul rand, verifica daca documentul este in
  cache. Daca este, modifica continutul acestuia si actualizeaza log-ul si
  raspunsul. Daca documentul nu este in cache, il cauta in baza de date. Daca
  nu este nici acolo, il adauga in cache si in baza de date. Daca este in
  database, il adauga in cache si actualizeaza continutul acestuia. In toate
  cazurile, se actualizeaza si lista de ordine, documentul fiind mutat la
  finalul acesteia (este cel mai recent folosit).

- `server_get_document`: Se ocupa de requesturile de tipul GET si returneaza
  response-ul corespunzator. In primul rand, verifica daca documentul este in
  cache. Daca este, actualizeaza log-ul, raspunsul este continutul documentului
  Daca documentul nu este in cache, il cauta in baza de date. Daca nu este nici
  acolo, actualizeaza log-ul si raspunsul este NULL. Daca este in baza de date,
  il adauga in cache si returneaza continutul acestuia. In toate cazurile, se
  actualizeaza si lista de ordine, documentul fiind mutat la finalul acesteia
  (este cel mai recent folosit). 

- `init_server`: Initializeaza un server cu un cache de dimensiune data si toate
  campurile acestuia. Creeaza un cache LRU, o coada pentru taskuri si o baza de
  date pentru documente.

- `server_handle_request`: Se ocupa de requesturile primite de la client si
  returneaza response-ul corespunzator. Verifica tipul request-ului si apeleaza
  functia corespunzatoare. Daca requestul este de tip EDIT, adauga requestul in
  coada si actualizeaza log-ul si raspunsul. Daca requestul este de tip GET,
  executa toate requesturile de tip EDIT din coada si apoi executa requestul de
  tip GET.

- `free_server`: Elibereaza memoria alocata pentru un server si toate campurile
  acestuia. Elibereaza memoria alocata pentru cache, coada si baza de date.
  De asemenea, elibereaza memoria alocata pentru lista dublu inlantuita si
  documentele din aceasta.