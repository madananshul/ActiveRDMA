#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

struct timing
{
    unsigned long long int
        jvm_nsec, insns, pkt, mem_rd, mem_wr, mem_cas;
};

#define CLIENT "10.0.0.2"
#define PORT 15713

int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    inet_pton(AF_INET, CLIENT, &(sin.sin_addr));

    // send an empty packet to the request port
    char buf[4];
    if (sendto(sockfd, buf, 0, 0, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("sendto");
        return 2;
    }

    // wait for response
    struct timing t;
    socklen_t addr_size = sizeof(sin);
    if (recvfrom(sockfd, &t, sizeof(t), 0, (struct sockaddr *)&sin, &addr_size) == -1)
    {
        perror("recvfrom");
        return 3;
    }

    // print values
    printf("# jvm_nsec insns pkts mem_rd mem_wr mem_cas\n");
    printf("%lld %lld %lld %lld %lld %lld\n",
            t.jvm_nsec, t.insns, t.pkt, t.mem_rd, t.mem_wr, t.mem_cas);

    return 0;
}
