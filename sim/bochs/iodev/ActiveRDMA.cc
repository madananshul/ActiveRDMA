#include "ActiveRDMA.h"
#include <jni.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

ActiveRDMA_c::ActiveRDMA_c()
{
}

ActiveRDMA_c::~ActiveRDMA_c()
{
}

void ActiveRDMA_c::handle_udp_packet(void *udp_data, int len)
{
}

void ActiveRDMA_c::send_udp_packet(void *udp_data, int len)
{
}

bool ActiveRDMA_c::handle_packet(void *data, int len)
{
    printf("packet: length %d\n", len);
    return false;
}
