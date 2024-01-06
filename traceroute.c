#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

double time_now_ms(void);
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

    const int MAX_TTL = 10;
    for (int ttl = 1; ttl < MAX_TTL; ++ttl) {
        if (setsockopt(sfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        char probe = '\0';
        if (send(sfd, &probe, sizeof(probe), 0) == -1) {
            perror("send");
            exit(1);
        }
        double send_ms = time_now_ms();

        char buf[512];
        if (recv(rfd, buf, sizeof(buf), MSG_WAITALL) == -1) {
            perror("recv");
            exit(1);
        }
        double recv_ms = time_now_ms();

        struct ip *ip_hdr = (struct ip *) buf;
        // Convert IP header length from 32-bit words to bytes. See ip(4).
        u_int ip_hlen = ip_hdr->ip_hl << 2;
        struct icmp *icmp_hdr = (struct icmp *) (buf + ip_hlen);

        u_char type = icmp_hdr->icmp_type;
        u_char code = icmp_hdr->icmp_code;

        if (type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) {
            const char *src_addr = inet_ntoa(ip_hdr->ip_src);
            double rtt_ms = (recv_ms - send_ms) / 2;
            printf("%s %f\n", src_addr, rtt_ms);

            if (strcmp(src_addr, host) == 0) {
                break;
            }
        }
    }

    close(sfd);
    close(rfd);

    return 0;
}

double time_now_ms(void)
{
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) == -1) {
        perror("clock_gettime");
        exit(1);
    }

    return (double) tp.tv_sec * 1e3 + (double) tp.tv_nsec * 1e-6;
}

void usage(void)
{
    fprintf(stderr, "usage: [sudo] traceroute host\n");
}
