#include <stdio.h>
#include <stdlib.h>

void usage(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
        exit(1);
    }

    const char *host = argv[1];
    (void) host;

    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: traceroute [host]\n");
}
