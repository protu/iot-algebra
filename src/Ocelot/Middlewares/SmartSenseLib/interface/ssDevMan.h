/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef __SS_DEVMAN_H
#define __SS_DEVMAN_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct netif_t netif_t;
struct netif_t
{
    int16_t (*socket_open)(int domain, int type, int protocol);
    int16_t (*socket_close)(netif_t *dev, int16_t s);
    int32_t (*socket_connect)(netif_t *dev, int socket, const struct sockaddr *addr, socklen_t addrlen);
    int32_t (*socket_send)(netif_t *dev, int socket, const char *buf, size_t len, int8_t flags);
    int32_t (*socket_sendto)(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         const struct sockaddr_in *to, socklen_t to_len);

    int32_t (*socket_recv)(netif_t *dev, int socket, char *buf, size_t len, int8_t flags);
    int32_t (*socket_recvfrom)(netif_t *dev, int s, char *buf, size_t len, int8_t flags,
                                         struct sockaddr_in *from, socklen_t *fromlen);
    int32_t (*gethostbyname)(const char *hostname, uint32_t *out_ip_addr);
};
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
netif_t *get_device(void);
//netif_t *s_init_wifi_dev(void);
//netif_t *s_init_modem(void);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SS_DEVMAN_H */
 
