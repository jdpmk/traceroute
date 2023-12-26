#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

void usage(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
        exit(1);
    }

    int sfd, rfd;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if ((rfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
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
    struct addrinfo *res;
    if ((ok = getaddrinfo(host, "80", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ok));
        exit(1);
    }

    if (connect(sfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    freeaddrinfo(res);

    // TODO: perform the following for multiple hops
    const int TTL = 1;
    if (setsockopt(sfd, IPPROTO_IP, IP_TTL, &TTL, sizeof(TTL)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    char probe = '\0';
    if (send(sfd, &probe, sizeof(probe), 0) == -1) {
        perror("send");
        exit(1);
    }

    char buf[512];
    if (recv(rfd, buf, sizeof(buf), 0) == -1) {
        perror("recvfrom");
        exit(1);
    }

    struct ip *ip_hdr = (struct ip *) buf;
    // IP header length in 32-bit words. See ip(4).
    u_int ip_hlen = ip_hdr->ip_hl << 2;
    struct icmp *icmp_hdr = (struct icmp *) (buf + ip_hlen);

    u_char type = icmp_hdr->icmp_type;
    u_char code = icmp_hdr->icmp_code;

    if (type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) {
        // TODO: print time taken for hop
    }

    close(sfd);
    close(rfd);

    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: [sudo] traceroute host\n");
}
