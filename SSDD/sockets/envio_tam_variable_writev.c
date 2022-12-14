#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

struct cabecera {
	int long1;
	int long2;
};

int main(int argc, char *argv[]) {
	int s, escrito;
	if (argc!=5) {
		fprintf(stderr, "Uso: %s servidor puerto dato1 dato2\n", argv[0]);
		return 1;
	}
	if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("error creando socket");
		return 1;
	}
        struct addrinfo *res;
        if (getaddrinfo(argv[1], argv[2], NULL, &res)!=0) {
                perror("error en getaddrinfo");
                close(s);
                return 1;
        }
        if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
                perror("error en connect");
                close(s);
                return 1;
        }
        freeaddrinfo(res);
	struct cabecera cab;
	cab.long1=htonl(strlen(argv[3]));
	cab.long2=htonl(strlen(argv[4]));
	struct iovec iov[3];
	iov[0].iov_base=&cab;
	iov[0].iov_len=sizeof(cab);
	iov[1].iov_base=argv[3];
	iov[1].iov_len=strlen(argv[3]);
	iov[2].iov_base=argv[4];
	iov[2].iov_len=strlen(argv[4]);
        if ((escrito=writev(s, iov, 3))<0) {
        	perror("error en writev");
        	close(s);
        	return 1;
        }
	printf("escrito %d\n", escrito);
	return 0;
}
