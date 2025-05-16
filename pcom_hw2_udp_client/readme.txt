Tema 2 - Protocoale de Comunicații

Autori: Nume Prenume, Grupa

------------------------------
Descriere protocol TCP:

Pentru comunicarea TCP între client și server folosim structura:

typedef struct {
    uint8_t type; // 0 = subscribe, 1 = unsubscribe, 2 = exit
    char topic[51]; // topic cu \0
    char client_id[11]; // doar la conectare
} __attribute__((packed)) tcp_message;

Clientul trimite această structură către server la fiecare comandă.

Mesajele de la server către client sunt transmise ca stringuri human-readable, deja formatate.

------------------------------
Comenzi suportate (client):

subscribe <TOPIC>
unsubscribe <TOPIC>
exit

------------------------------
Comandă server:

./server <PORT>

Comandă client:

./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>

------------------------------
Wildcard:

* – orice număr de nivele
+ – un singur nivel

------------------------------
Compilare:

make

Curățare:

make clean
