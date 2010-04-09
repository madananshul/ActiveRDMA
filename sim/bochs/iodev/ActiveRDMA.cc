#include "ActiveRDMA.h"
#include <jni.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define PROT_UDP 17

static int checksum(void *data, int len)
{
    int cksum = 0;
    short *ptr = (short *)data;

    while (len >= 2)
    {
        cksum += *ptr++;
        len -= 2;

        if (cksum > 0xffff)
            cksum = (cksum & 0xffff) + (cksum >> 16);
    }

    if (len)
        cksum += *(char *)ptr;

    while (cksum > 0xffff)
        cksum = (cksum & 0xffff) + (cksum >> 16);

    return ~cksum;
}

ActiveRDMA_c::ActiveRDMA_c()
{
    JavaVMInitArgs args;
    args.version = JNI_VERSION_1_4;
    JNI_GetDefaultJavaVMInitArgs(&args);

    m_jni = 0;
    m_jvm = 0;
    int err = JNI_CreateJavaVM(&m_jvm, (void **)&m_jni, &args);
}

ActiveRDMA_c::~ActiveRDMA_c()
{
}

bool ActiveRDMA_c::handle_udp_packet(unsigned char *udp_data, int len)
{
    udphdr *hdr = (udphdr *)udp_data;

    printf("UDP packet: src port %d, dst port %d, length %d\n",
            ntohs(hdr->source), ntohs(hdr->dest), ntohs(hdr->len));

    return false;
}

void ActiveRDMA_c::send_udp_packet(unsigned char *udp_data, int len)
{
}

bool ActiveRDMA_c::handle_ip_packet(unsigned char *ip_data, int len)
{
    iphdr *hdr = (iphdr *)ip_data;
    int src = ntohl(hdr->saddr), dest = ntohl(hdr->daddr);

    printf("IPv%d, header length %d, proto %d, saddr %d.%d.%d.%d, daddr %d.%d.%d.%d\n",
            hdr->version, ntohs(hdr->tot_len), hdr->protocol,
            src >> 24, (src >> 16) & 0xff, (src >> 8) & 0xff, src & 0xff,
            dest >> 24, (dest >> 16) & 0xff, (dest >> 8) & 0xff, dest & 0xff);

    if (hdr->protocol == PROT_UDP)
        return handle_udp_packet(ip_data + (4*hdr->ihl), len - (4*hdr->ihl));

    return false;
}

bool ActiveRDMA_c::handle_packet(void *data, int len)
{
    unsigned char *p = (unsigned char *)data;

    printf("Ethernet frame: src MAC %02x:%02x:%02x:%02x:%02x:%02x, dst MAC %02x:%02x:%02x:%02x:%02x:%02x, ethertype %02x:%02x\n",
            p[6], p[7], p[8], p[9], p[10], p[11], p[0], p[1], p[2], p[3], p[4], p[5], p[12], p[13]
          );

    if (p[12] == 0x08 && p[13] == 0x00) // 0x0800 is IPv4
        return handle_ip_packet(p + 14, len - 14 - 4);

    return false;
}
