#include "edsu.h"
#include "comun.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <string.h>


static int s;
static UUID_t uuid;

// se ejecuta antes que el main de la aplicación
__attribute__((constructor)) void inicio(void){
    if (begin_clnt()<0) {
        fprintf(stderr, "Error al iniciarse aplicación\n");
        // terminamos con error la aplicación antes de que se inicie
	// en el resto de la biblioteca solo usaremos return
        _exit(1);
    }
}

// se ejecuta después del exit de la aplicación
__attribute__((destructor)) void fin(void){
    if (end_clnt()<0) {
        fprintf(stderr, "Error al terminar la aplicación\n");
        // terminamos con error la aplicación
	// en el resto de la biblioteca solo usaremos return
        _exit(1);
    }
}

// operaciones que implementan la funcionalidad del proyecto
int begin_clnt(void){
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("error creando socket");
        return -1;
    }
    struct addrinfo *res;
    if (getaddrinfo(getenv("BROKER_HOST"), getenv("BROKER_PORT"), NULL, &res) != 0){
        perror("error en getaddrinfo");
        close(s);
        return -1;
    }
    if (connect(s, res->ai_addr, res->ai_addrlen) < 0){
        perror("error en connect");
        close(s);
        return -1;
    }
    generate_UUID(uuid);
    struct iovec iov[1];
    iov[0].iov_base = uuid;
    iov[0].iov_len = sizeof(UUID_t);
    if (writev(s, iov, 1)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev;
}

int end_clnt(void){
    int op=90;
    op=htonl(op);
    struct iovec iov[2];
    iov[0].iov_base=&op;
    iov[0].iov_len=sizeof(int);
	iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    if (writev(s, iov, 2)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    close(s);
    return dev;
}

int subscribe(const char *tema){
    int op=10;
    op=htonl(op);
    struct iovec iov[4];
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
	iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    int tam=strlen(tema);
    tam=htonl(tam);
    iov[2].iov_base=&tam;
	iov[2].iov_len=sizeof(int);
	iov[3].iov_base=(void *) tema;
	iov[3].iov_len=strlen(tema);
    if (writev(s, iov, 4)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev;
}

int unsubscribe(const char *tema){
    int op=20;
    op=htonl(op);
    struct iovec iov[4];
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
	iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    int tam=strlen(tema);
    tam=htonl(tam);
    iov[2].iov_base=&tam;
	iov[2].iov_len=sizeof(int);
	iov[3].iov_base=(void *) tema;
	iov[3].iov_len=strlen(tema);
    if (writev(s, iov, 4)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev;
}

int publish(const char *tema, const void *evento, uint32_t tam_evento){
    int op=60;
    op=htonl(op);
    struct iovec iov[6];
    iov[0].iov_base=&op;
    iov[0].iov_len=sizeof(int);
	iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    int tam=strlen(tema);
    tam=htonl(tam);
    iov[2].iov_base=&tam;
	iov[2].iov_len=sizeof(int);
	iov[3].iov_base=(void *) tema;
	iov[3].iov_len=strlen(tema);
    int tam1=htonl(tam_evento);
    iov[4].iov_base=&tam1;
	iov[4].iov_len=sizeof(uint32_t);
    iov[5].iov_base=(char *) evento;
	iov[5].iov_len=tam_evento;
    if (writev(s, iov, 6)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev; 
}

int get(char **tema, void **evento, uint32_t *tam_evento){
    int op=70;
    op=htonl(op);
    struct iovec iov[2];
    iov[0].iov_base=&op;
    iov[0].iov_len=sizeof(int);
	iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    if (writev(s, iov, 2)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int tam_tema;
    recv(s, &tam_tema, sizeof(int), MSG_WAITALL);
    tam_tema = ntohl(tam_tema);
    if(tam_tema==-1){
        return -1;
    }
    else{
        *tema=malloc(tam_tema+1);
        recv(s, *tema, tam_tema, MSG_WAITALL);
        tema[tam_tema]='\0';
        recv(s,tam_evento,sizeof(uint32_t),MSG_WAITALL);
        *tam_evento=ntohl(*tam_evento);
        *evento=malloc(*tam_evento);
        recv(s,*evento,*tam_evento,MSG_WAITALL);
        return 0;
    }
}

// operaciones que facilitan la depuración y la evaluación
int topics(){ // cuántos temas existen en el sistema
    struct iovec iov[2];
    int op=40;
    op=htonl(op);
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    if (writev(s, iov, 2)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev;
}

int clients(){ // cuántos clientes existen en el sistema
    struct iovec iov[2];
    int op=30;
    op=htonl(op);
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base=uuid;
	iov[1].iov_len=sizeof(UUID_t);
    if (writev(s, iov, 2)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev;
}

int subscribers(const char *tema){ // cuántos subscriptores tiene este tema
    struct iovec iov[4];
    int op=50;
    op=htonl(op);
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
    int tam=strlen(tema);
    tam=htonl(tam);
    iov[1].iov_base=&tam;
	iov[1].iov_len=sizeof(int);
	iov[2].iov_base=(void *) tema;
	iov[2].iov_len=strlen(tema);
    iov[3].iov_base=uuid;
	iov[3].iov_len=sizeof(UUID_t);
    if (writev(s, iov, 4)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev; 
}

int events() { // nº eventos pendientes de recoger por este cliente
    struct iovec iov[2];
    int op=80;
    op=htonl(op);
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = uuid;
    iov[1].iov_len = sizeof(UUID_t);
    if (writev(s, iov, 2)<0) {
        perror("error en writev");
        close(s);
        return -1;
    }
    int dev;
    recv(s, &dev, sizeof(int), MSG_WAITALL);
    dev = ntohl(dev);
    return dev; 
}