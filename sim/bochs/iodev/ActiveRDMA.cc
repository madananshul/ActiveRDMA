#include "ActiveRDMA.h"
#include <jni.h>
#include <string.h>
#include <netinet/ip.h> // struct iphdr
#include <netinet/udp.h> // struct udphdr

#define PROT_UDP 17
#define UDP_PORT 15712

//#define DUMP

static unsigned short checksum(unsigned char *data, int len, short init)
{
    unsigned int cksum = init;
    unsigned short *ptr = (unsigned short *)data;

    while (len >= 2)
    {
        cksum += *ptr++;
        len -= 2;
        data += 2;

        if (cksum > 0xffff)
            cksum = (cksum & 0xffff) + (cksum >> 16);
    }

    if (len)
    {
        unsigned char buf[2];
        buf[0] = *data;
        buf[1] = 0;
        cksum += *((unsigned short *)buf);
    }

    while (cksum > 0xffff)
        cksum = (cksum & 0xffff) + (cksum >> 16);

    return cksum;
}

ActiveRDMA_c::ActiveRDMA_c()
{
    JavaVMInitArgs args;
    JavaVMOption options[1];

    m_jni = 0;
    m_jvm = 0;

    args.version = JNI_VERSION_1_4;
    args.nOptions = 1;
    args.options = options;
    options[0].optionString = (char *) "-Djava.class.path=../../bin/";

    int err = JNI_CreateJavaVM(&m_jvm, (void **)&m_jni, &args);

    jclass cls = m_jni->FindClass("server/SimpleServer");
    jmethodID constructor = m_jni->GetMethodID(cls, "<init>", "(I)V");
    m_srv = m_jni->NewObject(cls, constructor, 4*1024*1024);

    m_srvMth = m_jni->GetMethodID(cls, "serve", "([B)[B");
}

ActiveRDMA_c::~ActiveRDMA_c()
{
}

static void dump(unsigned char *data, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");
}

void ActiveRDMA_c::handle_rdma_req(int src_addr, int src_port, unsigned char *data, int len)
{
    jbyteArray arr = m_jni->NewByteArray(len);
    m_jni->SetByteArrayRegion(arr, 0, len, (jbyte *)data);

    jbyteArray ret = (jbyteArray)m_jni->CallObjectMethod(m_srv, m_srvMth, arr);
    
    jboolean isCopy = false;
    jbyte *ret_data = m_jni->GetByteArrayElements(ret, &isCopy);

    send_udp_packet(src_addr, src_port, (unsigned char *)ret_data, m_jni->GetArrayLength(ret));
    if (isCopy)
        m_jni->ReleaseByteArrayElements(ret, ret_data, JNI_ABORT);
}

bool ActiveRDMA_c::handle_udp_packet(int src_addr, unsigned char *udp_data, int len)
{
    udphdr *hdr = (udphdr *)udp_data;

#ifdef DUMP
    printf("UDP packet: src port %d, dst port %d, length %d\n",
            ntohs(hdr->source), ntohs(hdr->dest), ntohs(hdr->len));
#endif

    if (ntohs(hdr->dest) == UDP_PORT)
    {
        handle_rdma_req(src_addr, ntohs(hdr->source), udp_data + 8, ntohs(hdr->len) - 8);
        return true;
    }

    return false;
}

void ActiveRDMA_c::send_udp_packet(int addr, int port, unsigned char *udp_data, int len)
{
    // build an Ethernet frame
    unsigned char buf[2048];
    memset(buf, 0, sizeof(buf));

    // Ethernet header
    memcpy((void *)&buf[0], (void *)m_partner_eth, 6);
    memcpy((void *)&buf[6], (void *)m_my_eth, 6);
    buf[12] = 0x08; // 0x0800 == IPv4 over Ethernet
    buf[13] = 0x00;

    // IPv4 header
    iphdr *hdr = (iphdr *)(&buf[14]);
    memset((void *)hdr, 0, sizeof(iphdr));
    hdr->ihl = 5;
    hdr->version = 4;
    hdr->ttl = 255;
    hdr->protocol = PROT_UDP;

    hdr->saddr = htonl(m_my_ip);
    hdr->daddr = htonl(addr);

    hdr->tot_len = htons(20 + 8 + len);

    // IPv4 checksum
    hdr->check = 0;
    hdr->check = ~ checksum((unsigned char *)hdr, sizeof(iphdr), 0);
    if (hdr->check == 0)
        hdr->check = 0xFFFF;

    // UDP header
    udphdr *udp = (udphdr *)(&buf[34]);
    memset((void *)udp, 0, sizeof(udphdr));
    udp->source = htons(UDP_PORT);
    udp->dest = htons(port);
    udp->len = htons(8 + len);
    udp->check = 0; // UDP checksum is optional

    // payload
    memcpy((void *)(&buf[42]), udp_data, len);

    // send it!
    int tot_len = 14 + 20 + 8 + len;
    if (tot_len < 60) tot_len = 60;

    m_sender(m_sender_p, (void *)buf, tot_len);
}

bool ActiveRDMA_c::handle_ip_packet(unsigned char *ip_data, int len)
{
    iphdr *hdr = (iphdr *)ip_data;
    int src = ntohl(hdr->saddr), dest = ntohl(hdr->daddr);

    m_my_ip = dest;

#ifdef DUMP
    printf("IPv%d, length %d, proto %d, saddr %d.%d.%d.%d, daddr %d.%d.%d.%d\n",
            hdr->version, ntohs(hdr->tot_len), hdr->protocol,
            src >> 24, (src >> 16) & 0xff, (src >> 8) & 0xff, src & 0xff,
            dest >> 24, (dest >> 16) & 0xff, (dest >> 8) & 0xff, dest & 0xff);

    unsigned short check = hdr->check;
    printf("real checksum is %04x, ", hdr->check);
    hdr->check = 0;
    printf("computed checksum is %04x\n", (unsigned int) ~checksum((unsigned char *)hdr, hdr->ihl * 4, 0));
    hdr->check = check;
#endif

    if (hdr->protocol == PROT_UDP)
        return handle_udp_packet(src, ip_data + (4*hdr->ihl), len - (4*hdr->ihl));

    return false;
}

bool ActiveRDMA_c::handle_packet(void *data, int len)
{
    unsigned char *p = (unsigned char *)data;

#ifdef DUMP
    printf("Ethernet frame (length %d): src MAC %02x:%02x:%02x:%02x:%02x:%02x, dst MAC %02x:%02x:%02x:%02x:%02x:%02x, ethertype %02x:%02x\n",
            len,
            p[6], p[7], p[8], p[9], p[10], p[11], p[0], p[1], p[2], p[3], p[4], p[5], p[12], p[13]
          );
#endif

    if (p[12] == 0x08 && p[13] == 0x00) // 0x0800 is IPv4
    {
        memcpy((void *)m_my_eth, (void *)p, 6);
        memcpy((void *)m_partner_eth, (void *)(p + 6), 6);

        return handle_ip_packet(p + 14, len - 14);
    }

    return false;
}
