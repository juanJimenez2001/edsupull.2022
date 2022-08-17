#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include "queue.h"
#include "set.h"
#include "map.h"
#include "comun.h"
#include <pthread.h>
#include <fcntl.h>


typedef struct cliente {
    char *uuid; 
    set *temas;      
    queue *eventos;
} cliente;

typedef struct tema {
    const char *nombre_tema;     
    set *subs;
} tema;

typedef struct evento {
    char *tema;
    char *event;
    uint32_t tam;
    int contador;
} evento;

static map *mapa_uuid;
static map *mapa_temas;


void *servicio(void *arg){
    int s_srv, tam;
    int error;
    int dev=0;
    UUID_t uuid;
    char *temaT;
    struct tema * t;
    struct cliente * c;
    struct evento *e;
    s_srv = (long) arg;
    int op;  
    char *event;
    uint32_t tam_evento;

    struct iovec iov[1];
    printf("Cliente nuevo\n");
    recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
    c = malloc(sizeof(cliente));
    c->uuid = uuid;
    c->temas = set_create(0);
    c->eventos = queue_create(0);
    map_put(mapa_uuid, c->uuid, c);
    dev=htonl(dev);
    iov[0].iov_base = &dev;
    iov[0].iov_len = sizeof(int);
    writev(s_srv, iov, 1);

    while(recv(s_srv, &op, sizeof(int), MSG_WAITALL)>0){
        op = ntohl(op);

    //Subscripcion
    //En caso de error escribir al cliente y tb cuando termine
        if(op==10){
            dev=0;
            printf("peticion subscripcion\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            recv(s_srv, &tam, sizeof(int), MSG_WAITALL);
            tam=ntohl(tam);
            temaT= malloc(tam+1);
            recv(s_srv, temaT, tam, MSG_WAITALL);
            temaT[tam]='\0';
            t = map_get(mapa_temas, temaT, &error);
            if (error==-1){
                fprintf(stderr, "error: el tema no existe\n");
                dev=-1;
            } 
            else{
                c = map_get(mapa_uuid, uuid, &error);
                if (error==-1){
                    fprintf(stderr, "error: el usuario no existe\n");
                    dev=-1;
                } 
                else{
                    if (set_add(c->temas, t)<0){
                        fprintf(stderr, "error: persona ya esta subscrito al tema\n");
                        dev=-1;
                    }
                    if (set_add(t->subs, c)<0){
                        fprintf(stderr, "error: tema ya incluye a esa persona\n");
                        dev=-1;
                    }
                }
            }
            dev=htonl(dev);
            iov[0].iov_base = &dev;
            iov[0].iov_len = sizeof(int);
            writev(s_srv, iov, 1);
        }

    //Baja Subscripcion 
    //Return o next??
        else if(op==20){
            dev=0;
            printf("peticion desubscripcion\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            recv(s_srv, &tam, sizeof(int), MSG_WAITALL);
            tam=ntohl(tam);
            temaT=malloc(tam+1);
            recv(s_srv, temaT, tam, MSG_WAITALL);
            temaT[tam]='\0';
            t = map_get(mapa_temas, temaT, &error);
            if (error==-1){
                fprintf(stderr, "error: el tema no existe\n");
                dev=-1;
            } 
            else{
                c = map_get(mapa_uuid, uuid, &error);
                if (error==-1){
                    fprintf(stderr, "error: el usuario no existe\n");
                    dev=-1;
                }
                else{
                    if(set_contains(c->temas, t)==0){
                        fprintf(stderr, "error: el usuario no esta subscrito\n");
                        dev=-1;
                    }
                    else{
                        set_remove(t->subs, c, NULL);
                        set_remove(c->temas, t, NULL);
                    }
                }  
            } 
            dev=htonl(dev);
            iov[0].iov_base = &dev;
            iov[0].iov_len = sizeof(int);
            writev(s_srv, iov, 1);
        }

    //Clients
        else if(op==30){
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            int dato;
            struct iovec iov[1];
            printf("peticion clients\n");
            dato=htonl(map_size(mapa_uuid));
            iov[0].iov_base = &dato;
            iov[0].iov_len = sizeof(int);
            if (writev(s_srv, iov, 1)<0) {
                perror("error en writev");
            }
        }

    //Topics
        else if(op==40){
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            int dato;
            struct iovec iov[1];
            printf("peticion topics\n");
            dato=htonl(map_size(mapa_temas));
            iov[0].iov_base = &dato;
            iov[0].iov_len = sizeof(int);
            if (writev(s_srv, iov, 1)<0) {
                perror("error en writev");
            }
        }

    //Subscribers
        else if(op==50){
            dev=0;
            printf("peticion subscribers\n");
            recv(s_srv, &tam, sizeof(int), MSG_WAITALL);
            tam=ntohl(tam);
            temaT=malloc(tam+1);
            recv(s_srv, temaT, tam, MSG_WAITALL);
            temaT[tam]='\0';
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            t = map_get(mapa_temas, temaT, &error);
            if (error==-1){
                dev=-1;
                fprintf(stderr, "error: el tema no existe\n");
                dev=htonl(dev);
                iov[0].iov_base = &dev;
                iov[0].iov_len = sizeof(int);
                if (writev(s_srv, iov, 1)<0) {
                    perror("error en writev");
                }
            }
            else{
                int dato=set_size(t->subs);;
                struct iovec iov[1];
                dato=htonl(dato);
                iov[0].iov_base = &dato;
                iov[0].iov_len = sizeof(int);
                if (writev(s_srv, iov, 1)<0) {
                    perror("error en writev");
                }
            }
        }

    //Publicacion
        else if(op==60){
            dev=0;
            printf("peticion publicacion\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            recv(s_srv, &tam, sizeof(int), MSG_WAITALL);
            tam=ntohl(tam);
            temaT=malloc(tam+1);
            recv(s_srv, temaT, tam, MSG_WAITALL);
            temaT[tam]='\0';
            recv(s_srv,&tam_evento,sizeof(uint32_t),MSG_WAITALL);
            tam_evento=ntohl(tam_evento);
            event=malloc(tam_evento);
            recv(s_srv,event,tam_evento,MSG_WAITALL);
            
            t = map_get(mapa_temas, temaT, &error); 
            if (error==-1){
                dev=-1;
                fprintf(stderr, "error: el tema no existe\n");
            } 
            else{
                e = malloc(sizeof(evento));
                e->tema = temaT;
                e->event = event;
                e->tam = tam_evento;
                e->contador=set_size(t->subs);
                set_iter *i = set_iter_init(t->subs);
                for ( ; set_iter_has_next(i); set_iter_next(i)) {
                ++e->contador;
                    struct cliente *cli = ((cliente *)(set_iter_value(i)));
                    queue_push_back(cli->eventos, e);
                }
                set_iter_exit(i);
            }
            dev=htonl(dev);
            iov[0].iov_base = &dev;
            iov[0].iov_len = sizeof(int);
            writev(s_srv, iov, 1);
        }

    //Get
        else if(op==70){
            struct iovec iov1[4];
            dev=0;
            printf("peticion get\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            c = map_get(mapa_uuid, uuid, &error);
            if (error==-1){
                dev=-1;
                dev=htonl(dev);
                iov1[0].iov_base=&dev;
                iov1[0].iov_len=sizeof(int);
                if (writev(s_srv, iov1, 1)<0) {
                    perror("error en writev");
                }
            } 
            else{
                e=queue_pop_front(c->eventos, NULL);
                if (--e->contador==0)
                    free(e);
                tam=strlen(e->tema);
                tam=htonl(tam);
                iov1[0].iov_base=&tam;
                iov1[0].iov_len=sizeof(int);
                iov1[1].iov_base=e->tema;
                iov1[1].iov_len=strlen(e->tema);
                int tam1=htonl(e->tam);
                iov1[2].iov_base=&tam1;
                iov1[2].iov_len=sizeof(uint32_t);
                iov1[3].iov_base=e->event;
                iov1[3].iov_len=e->tam;
                if (writev(s_srv, iov1, 4)<0) {
                    perror("error en writev");
                }
            }
        }

    //Events
        else if(op==80){
            printf("peticion events\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            c=map_get(mapa_uuid,uuid, &error);
            int dato;
            if (error==-1) dato=-1;
            else dato=queue_length(c->eventos);
            dato=htonl(dato);
            iov[0].iov_base = &dato;
            iov[0].iov_len = sizeof(int);
            if (writev(s_srv, iov, 1)<0) {
                perror("error en writev");
            } 
        }

    //End client
        else if(op==90){
            dev=0;
            printf("peticion end client\n");
            recv(s_srv, &uuid, sizeof(UUID_t), MSG_WAITALL);
            c = map_get(mapa_uuid, uuid, &error);
            if (error==-1){
                fprintf(stderr, "error: el cliente no existe\n");
                dev=-1;
            } 
            else{
                set_iter *i = set_iter_init(c->temas);
                for ( ; set_iter_has_next(i); set_iter_next(i)) {
                    tema *tem = (tema *)set_iter_value(i);
                    set_remove(tem->subs, c, NULL);
                }
                set_iter_exit(i);
                map_remove_entry(mapa_uuid,uuid, NULL); 
                set_destroy(c->temas,NULL);
                queue_destroy(c->eventos,NULL);
            }
            dev=htonl(dev);
            iov[0].iov_base = &dev;
            iov[0].iov_len = sizeof(int);
            if (writev(s_srv, iov, 1)<0) {
                perror("error en writev");
            } 
        }
    }
    free(temaT);
    free(event);
	close(s_srv);  
	return NULL;
}

int main(int argc, char *argv[]){
    int s, s_conec;
    int opcion=1;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    if(argc!=3) {
        fprintf(stderr, "Uso: %s puerto fichero_temas\n", argv[0]);
        return -1;
    }
    mapa_uuid = map_create(key_string, 1);
    mapa_temas = map_create(key_string, 1);
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket");
        return -1;
    }
    
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion))<0){
        perror("error en setsockopt");
        return -1;
    }

    dir.sin_addr.s_addr=INADDR_ANY;
    dir.sin_port=htons(atoi(argv[1]));
    dir.sin_family=PF_INET;
    if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
        perror("error en bind");
        close(s);
        return -1;
    }
    if (listen(s, 5) < 0) {
        perror("error en listen");
        close(s);
        return -1;
    }

    FILE* temasF = fopen(argv[2], "r");
    if (!temasF)
        return -1;
    char *nombreTema = NULL;
    while (fscanf(temasF,"%ms", &nombreTema)>0){
        tema *t = malloc(sizeof(tema));
        t->nombre_tema = nombreTema;
        t->subs = set_create(0);
        map_put(mapa_temas, t->nombre_tema, t);
    }
    fclose(temasF);

    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); 
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);
    while(1) {
        tam_dir=sizeof(dir_cliente);
        if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
            perror("error en accept");
            close(s);
            return -1;
        }
	    pthread_create(&thid, &atrib_th, servicio, (void *)(long)s_conec);
    }
    close(s);
    return 0;
}