# SHA256
Il progetto consite nel realizzare un server che permetta multiple computazioni di impronte SHA-256. 

Il tempo di calcolo per impronta è proporzionale al numero di byte dell’ingresso (ovvero il file), dipendente dalla piattaforma e dalla implementazione dell’algoritmo (∼1 secondo per 70 MB su laptop moderno nella implementazione OpenSSL, ∼1 secondo per 30 KB su MentOS in QEMU). 

Successivamente va realizzato un client che invii l’informazione di file di input al server e riceva l’impronta risultante appena computata.
