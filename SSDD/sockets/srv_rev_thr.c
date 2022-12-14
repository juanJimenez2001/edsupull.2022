#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

void revierte(char *b, int t){
    char aux;
    for (int i=0; i<t/2; i++) {
        aux=b[i];
        b[i]=b[t-i-1];
        b[t-i-1]=aux;
    }
}
void *servicio(void *arg){
        int s_srv, tam;
        s_srv=(long) arg;
        printf("nuevo cliente\n");
        while (recv(s_srv, &tam, sizeof(tam), MSG_WAITALL)>0) {
            printf("recibida petición cliente\n");
            int tamn=ntohl(tam);
            char *dato = malloc(tamn);
            recv(s_srv, dato, tamn, MSG_WAITALL);
            sleep(5); // para probar que servicio no es concurrente
            revierte(dato, tamn);
            send(s_srv, dato, tamn, 0);
        }
        close(s_srv);
	return NULL;
}

int main(int argc, char *argv[]) {
    int s, s_conec;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    int opcion=1;

    if (argc!=2) {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return 1;
    }
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket");
        return 1;
    }
    /* Para reutilizar puerto inmediatamente */
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion))<0){
                perror("error en setsockopt");
                return 1;
        }
    dir.sin_addr.s_addr=INADDR_ANY;
    dir.sin_port=htons(atoi(argv[1]));
    dir.sin_family=PF_INET;
    if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
        perror("error en bind");
        close(s);
        return 1;
    }
    if (listen(s, 5) < 0) {
        perror("error en listen");
        close(s);
        return 1;
    }
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); // evita pthread_join
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);
    while(1) {
        tam_dir=sizeof(dir_cliente);
        if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
            perror("error en accept");
            close(s);
            return 1;
        }
	pthread_create(&thid, &atrib_th, servicio, (void *)(long)s_conec);
    }
    close(s);
    return 0;
}
