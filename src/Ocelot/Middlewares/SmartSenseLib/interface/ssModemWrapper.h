/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef __SS__MODEMWRAPPER_H
#define __SS__MODEMWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ssDevMan.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
static int16_t mdm_socket_open(int domain, int type, int protocol);
static int16_t mdm_socket_close(netif_t *dev, int16_t socket);
static int32_t mdm_connect(netif_t *dev, int socket, const struct sockaddr *addr, socklen_t addrlen);
static int32_t mdm_send(netif_t *dev, int socket, const char *buf, size_t len, int8_t flags);
static int32_t mdm_sendto(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         const struct sockaddr_in *to, socklen_t to_len);

static int32_t mdm_recv(netif_t *dev, int socket, char *buf, size_t len, int8_t flags);
static int32_t mdm_recvfrom(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         struct sockaddr_in *from, socklen_t *fromlen);

static int32_t mdm_gethostbyname(char const *hostname, uint32_t *out_ip_addr);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SS__MODEMWRAPPER_H */
 
