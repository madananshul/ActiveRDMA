#include "ActiveRDMA.h"
#include <jni.h>
#include <string.h>
#include <netinet/ip.h> // struct iphdr
#include <netinet/udp.h> // struct udphdr
#include <net/if_arp.h> // struct arphdr
#include <time.h> // clock_gettime()

#define PROT_UDP 17
#define UDP_PORT 15712
#define MEMORY_SIZE 128*1024*1024

//#define DUMP

ActiveRDMA_c *ActiveRDMA_c::singleton = 0;

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

ActiveRDMA_c::ActiveRDMA_c(char *macaddr)
{
    JavaVMInitArgs args;
    JavaVMOption options[1];

    m_jni = 0;
    m_jvm = 0;

    args.version = JNI_VERSION_1_4;
    args.nOptions = 1;
    args.options = options;
    options[0].optionString = (char *) "-Djava.class.path=../../build/";

    int err = JNI_CreateJavaVM(&m_jvm, (void **)&m_jni, &args);

    m_cls = m_jni->FindClass("server/SimpleServer");
    jmethodID constructor = m_jni->GetMethodID(m_cls, "<init>", "(I)V");
    m_srv = m_jni->NewObject(m_cls, constructor, MEMORY_SIZE);

    m_srvMth = m_jni->GetMethodID(m_cls, "serve", "([B)[B");
    m_getMth = m_jni->GetMethodID(m_cls, "getStat", "(I)J");

    m_timing.jvm_nsec = m_timing.insns = m_timing.pkt =
        m_timing.mem_rd = m_timing.mem_wr = m_timing.mem_cas = 0;

    singleton = this;

    m_my_ip = (10 << 24) | (0 << 16) | (0 << 8) | (2 << 0);
    m_my_eth = (unsigned char *)macaddr;
}

ActiveRDMA_c::~ActiveRDMA_c()
{
}

void ActiveRDMA_c::update_mem_stats()
{
    m_timing.mem_rd = m_jni->CallLongMethod(m_srv, m_getMth, 0);
    m_timing.mem_wr = m_jni->CallLongMethod(m_srv, m_getMth, 1);
    m_timing.mem_cas = m_jni->CallLongMethod(m_srv, m_getMth, 2);
}

void ActiveRDMA_c::handle_rdma_req(int src_addr, int src_port, unsigned char *data, int len)
{
    struct timespec t1, t2;

    jbyteArray arr = m_jni->NewByteArray(len);
    m_jni->SetByteArrayRegion(arr, 0, len, (jbyte *)data);

    clock_gettime(CLOCK_REALTIME, &t1);
    jbyteArray ret = (jbyteArray)m_jni->CallObjectMethod(m_srv, m_srvMth, arr);
    clock_gettime(CLOCK_REALTIME, &t2);

    long long int nsec = 1000000000 * (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec);

    m_timing.jvm_nsec += nsec;
    
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

    if (ntohs(hdr->dest) == UDP_PORT) // port 15712: call up to SimpleServer
    {
        handle_rdma_req(src_addr, ntohs(hdr->source), udp_data + 8, ntohs(hdr->len) - 8);
        return true; // do not pass to host (intercept)
    }
    if (ntohs(hdr->dest) == UDP_PORT + 1) // port 15713: return timing information
    {
        update_mem_stats();
        send_udp_packet(src_addr, ntohs(hdr->source), (unsigned char *)&m_timing, sizeof(m_timing));
        return true;
    }

    return false; // if not intercepted, give packet to host
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

    if (dest != m_my_ip)
        return false;

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

bool ActiveRDMA_c::handle_arp_packet(unsigned char *eth, unsigned char *arp_data, int len)
{
    arphdr *arp = (arphdr *)arp_data;
    int ip = htonl(m_my_ip);

    if (ntohs(arp->ar_op) == ARPOP_REQUEST &&
            !memcmp(arp_data + sizeof(arphdr) + 10 + 6, (void *)&ip, 4))
    {
        printf("got a match! sending ARP reply.\n");

        // copy sender MAC/IP to receiver fields
        char temp[10];
        memcpy(arp_data + sizeof(arphdr) + 10, arp_data + sizeof(arphdr), 10);
        // this is a reply
        arp->ar_op = htons(ARPOP_REPLY);
        // copy our IP and MAC address in
        memcpy(arp_data + sizeof(arphdr), m_my_eth, 6);
        memcpy(arp_data + sizeof(arphdr) + 6, &ip, 4);

        // swap source and dest ethernet addrs in eth frame
        memcpy(eth, eth + 6, 6);
        memcpy(eth + 6, m_my_eth, 6);

        // send ethernet frame
        m_sender(m_sender_p, eth, 14 + sizeof(arphdr) + 20);

        return true;
    }

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
        memcpy((void *)m_partner_eth, (void *)(p + 6), 6);

        m_timing.pkt++;

        return handle_ip_packet(p + 14, len - 14);
    }

    if (p[12] == 0x08 && p[13] == 0x06) // 0x0806 is ARP
    {
        return handle_arp_packet(p, p + 14, len - 14);
    }

    return false;
}
