// traceroute.c
// A traceroute program implementation in C.
//
// Joydeep Mukherjee
// 2023/12/25
//
// For more information, please see README.md and LICENSE.md.

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

#define TR_LINE_FMT "%-5s %-60s %-20s\n"  // format to log hops, with padding
#define MAX_TTL 64                        // maximum no. of network hops

double time_now_ms(void);
void usage(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
        exit(1);
    }

    // Setup sockets to send probe packets to the destination, and receive ICMP
    // packets from each hop. The sending packet connects to the host port 80
    // via TCP. This port is typically used for HTTP connections. We avoid UDP
    // here for various reasons like firewalls, etc. See traceroute(8).
    int sfd;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket (send)");
        exit(1);
    }

    int rfd;
    if ((rfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        perror("socket (rcv)");
        exit(1);
    }

    const char *host = argv[1];

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

    printf("traceroute to %s (%d hops max)\n", host, MAX_TTL);
    printf(TR_LINE_FMT, "Hop", "IP", "Time (ms)");
    printf(TR_LINE_FMT, "---", "--", "---------");

    for (int ttl = 1; ttl <= MAX_TTL; ++ttl) {
        // Set the IP TTL.
        if (setsockopt(sfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Send a null byte as a probe to the host, and await a response.
        // TODO: Maybe implement retries, assuming ICMP packets may get lost?
        char probe = '\0';
        if (send(sfd, &probe, sizeof(probe), 0) == -1) {
            perror("send");
            exit(1);
        }
        double send_ms = time_now_ms();

        // TODO: 512 bytes is probably enough here.
        char buf[512];
        if (recv(rfd, buf, sizeof(buf), MSG_WAITALL) == -1) {
            perror("recv");
            exit(1);
        }
        double recv_ms = time_now_ms();

        // Extract the ICMP header from the received response. We convert the
        // IP header length from 32-bit words to bytes. See ip(4).
        struct ip *ip_hdr = (struct ip *) buf;
        u_int ip_hlen = ip_hdr->ip_hl << 2;
        struct icmp *icmp_hdr = (struct icmp *) (buf + ip_hlen);

        u_char type = icmp_hdr->icmp_type;
        u_char code = icmp_hdr->icmp_code;

        if (type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) {
            char hop_str[255] = {0};
            char ip_str[255] = {0};
            char latency_ms_str[255] = {0};

            // Hop number is the same as current IP TTL.
            sprintf(hop_str, "%d", ttl);

            // Latency is RTT / 2.
            double latency_ms = (recv_ms - send_ms) / 2;
            sprintf(latency_ms_str, "%f", latency_ms);

            // Parse the IP address of the hop.
            const char *hop_addr = inet_ntoa(ip_hdr->ip_src);
            struct in_addr ip;
            if (inet_aton(hop_addr, &ip) == 0) {
                fprintf(stderr,
                        "ERROR: unable to interpret %s as an IP address",
                        hop_addr);
                exit(1);
            }

            // Get the name of this hop, if available.
            // NULL on error, in which case we don't log the hostname.
            struct hostent *he;
            if ((he = gethostbyaddr(&ip, sizeof(ip), AF_INET))) {
                sprintf(ip_str, "%s (%s)", hop_addr, he->h_name);
            } else {
                sprintf(ip_str, "%s", hop_addr);
            }

            // Log this hop.
            printf(TR_LINE_FMT, hop_str, ip_str, latency_ms_str);

            // Reached destination.
            if (strcmp(hop_addr, host) == 0) {
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
