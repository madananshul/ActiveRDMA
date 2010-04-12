#ifndef BX_IODEV_ACTIVERDMA
#define BX_IODEV_ACTIVERMDA

#include <jni.h>

class ActiveRDMA_c
{
    public:
        typedef void (*packet_sender)(void *p, void *data, int len);

    private:
        JNIEnv *m_jni;
        JavaVM *m_jvm;
        jobject m_srv;
        jmethodID m_srvMth;

        char *m_mem;
        int m_mem_size;

        packet_sender m_sender;
        void *m_sender_p;

        // we assume a point-to-point TUN/TAP link and grab eth addrs from UDP packets
        unsigned char m_my_eth[6], m_partner_eth[6];

        // we pick this up from whatever the client calls us (``oh, OK, I'm 10.0.0.2!'')
        int m_my_ip;

        void handle_rdma_req(int src_addr, int port, unsigned char *data, int len);
        bool handle_udp_packet(int ip, unsigned char *udp_data, int len);
        void send_udp_packet(int ip, int port, unsigned char *udp_data, int len);

        bool handle_ip_packet(unsigned char *ip_data, int len);

        static const int PORT = 15712;

    public:

        ActiveRDMA_c();
        virtual ~ActiveRDMA_c();

        bool handle_packet(void *data, int len);
        void reg_sender(packet_sender snd, void *p) { m_sender = snd; m_sender_p = p; }
};

#endif
