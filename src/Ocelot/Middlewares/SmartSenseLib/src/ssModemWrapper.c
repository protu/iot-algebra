/**
* @file     
* @brief    
* @warning
* @details
*
* Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
*
**/

/*------------------------- INCLUDED FILES ************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
  
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
//#include "ssSocket.h"
#include "assert.h"
#include "ssDevMan.h"
#include "ATCmdParser.h"
#include "ssModem.h"
#include "ssModemWrapper.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
//#define APN       "data.vip.hr"
//#define PIN "9910"
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
uint8_t modem_flag = 0;
modem_t *modem;
/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static int16_t mdm_socket_open(int domain, int type, int protocol)
{

    return modem_socket_open(modem, protocol);
}


static int16_t mdm_socket_close(netif_t *dev, int16_t socket)
{

    return modem_socket_close((modem_t *)dev, socket);
}

static int32_t mdm_connect(netif_t *dev, int socket, const struct sockaddr *addr, socklen_t addrlen)
{
    int16_t result = 0;
    struct SocketAddress_in address;
    uint32_t addr_in = ((struct sockaddr_in *) addr)->sin_addr.s_addr;

    address.sin_family = ((struct sockaddr_in *)addr)->sin_family;
    address.sin_port = ntohs(((struct sockaddr_in *)addr)->sin_port);
    inet_ntop(AF_INET, &addr_in, address.sin_addr, INET_ADDRSTRLEN);

    result = modem_socket_connect((modem_t *)dev, socket, &address);

    return result;

}

static int32_t mdm_send(netif_t *dev, int socket, const char *buf, size_t len, int8_t flags)
{
    int32_t result = 0;

    result = modem_socket_send((modem_t *)dev, socket, buf, len);

    return result;

}

static int32_t mdm_sendto(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         const struct sockaddr_in *to, socklen_t to_len)
{
    int32_t result;
    struct SocketAddress_in address;
    uint32_t addr = to->sin_addr.s_addr;

    address.sin_family = to->sin_family;
    address.sin_port = htons(to->sin_port);
    inet_ntop(AF_INET, &addr, address.sin_addr, INET_ADDRSTRLEN);

    result = modem_socket_sendto((modem_t *)dev, s, buf, len, &address);

    return result;
}

static int32_t mdm_recv(netif_t *dev, int socket, char *buf, size_t len, int8_t flags)
{
    int32_t result = 0;

    result = modem_socket_recv((modem_t *)dev, socket, buf, (uint8_t)len);

    return result;
}
static int32_t mdm_recvfrom(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         struct sockaddr_in *from, socklen_t *fromlen)
{
    int32_t result = 0;
    struct SocketAddress_in address;

    result = modem_socket_recvfrom((modem_t *)dev, s, buf, len, &address);

    //address.sin_family = AF_INET;
    from->sin_family = address.sin_family;
    from->sin_port = htons(address.sin_port);
    inet_aton(address.sin_addr, &from->sin_addr);

    return result;
}
static int32_t mdm_gethostbyname(char const *hostname, uint32_t *out_ip_addr)
{
  return modem_gethostbyname(modem, hostname, out_ip_addr);
}

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
netif_t *s_init_modem(const char *apn, const char *pin)
{
  if (!modem_flag)
  {
    modem = modem_create();
    
    modem->com_dev.socket_open     = mdm_socket_open;
    modem->com_dev.socket_close    = mdm_socket_close;
    modem->com_dev.socket_connect  = mdm_connect;
    modem->com_dev.socket_send     = mdm_send;
    modem->com_dev.socket_sendto   = mdm_sendto;
    modem->com_dev.socket_recv     = mdm_recv;
    modem->com_dev.socket_recvfrom = mdm_recvfrom;
    modem->com_dev.gethostbyname   = mdm_gethostbyname;
    assert(modem);
    assert(modem_init(modem, pin));
    assert(modem_nwk_register(modem));
    modem_set_credentials(modem, apn, NULL, NULL);
    assert(modem_nwk_connect(modem));
    modem_flag = 1;
  }

  return (netif_t *) modem;
}

#ifdef __cplusplus
}
#endif
