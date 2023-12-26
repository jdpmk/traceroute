#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>

void usage(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
        exit(1);
    }

    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    const char *host = argv[1];
    printf("traceroute to %s\n", host);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ok;
    struct addrinfo* res;
    if ((ok = getaddrinfo(host, "80", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ok));
        exit(1);
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    freeaddrinfo(res);

    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: traceroute [host]\n");
}
