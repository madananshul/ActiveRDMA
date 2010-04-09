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

        char *m_mem;
        int m_mem_size;

        packet_sender m_sender;
        void *m_sender_p;

        bool handle_udp_packet(unsigned char *udp_data, int len);
        void send_udp_packet(unsigned char *udp_data, int len);

        bool handle_ip_packet(unsigned char *ip_data, int len);

        static const int PORT = 15712;

    public:

        ActiveRDMA_c();
        virtual ~ActiveRDMA_c();

        bool handle_packet(void *data, int len);
        void reg_sender(packet_sender snd, void *p) { m_sender = snd; m_sender_p = p; }
};

#endif
