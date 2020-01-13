/**
 * @file
 * @brief
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef SS_SOCKET_H
#define SS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif
  
/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define NUM_SOCKETS    7
#define MAX_SOCKET_ADDRESS 16
#define SOCKBUF_SIZE 512
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef uint16_t address_family_t;
typedef unsigned int flag_t;

/**************************************************************************/ /*!
 * @brief General return codes, used by most of API functions.
 ******************************************************************************/
typedef enum
{
    STATUS_OK  = (0), /**< No error.*/
    STATUS_ERR = (-1) /**< There is error.*/
} err_return_t;
/**************************************************************************/ /*!
 * @brief Boolean type.
 ******************************************************************************/
typedef enum
{
    SS_FALSE = 0, /**< FALSE Boolean value.*/
    SS_TRUE  = 1  /**< TRUE Boolean value.*/
} bool_t;



/**************************************************************************/ /*!
 * @brief Socket state.
 ******************************************************************************/
typedef enum
{
    SS_UNCONNECTED = (0),   /**< @brief Not connected to any socket.*/
    SS_CONNECTING  = (1),   /**< @brief In process of connecting.*/
    SS_CONNECTED   = (2),   /**< @brief Connected to a socket.*/
    SS_LISTENING   = (3)    /**< @brief In listening state.*/
} socket_state_t;

/**************************************************************************/ /*!
 * @brief Socket types.
 ******************************************************************************/
typedef enum
{
    SS_SOCK_UNSPEC = (0U),  /**< @brief Unspecified socket type.
                         */
    SS_SOCK_STREAM = (1U),  /**< @brief Stream socket.@n
                         * Provides reliable, two-way, connection-based
                         * byte stream. It corresponds to the TCP protocol
                         */
    SS_SOCK_DGRAM  = (2U),  /**< @brief Datagram socket.@n
                         * Provides unreliable, connectionless datagrams.
                         * It corresponds to the UDP protocol.
                         */
    SS_SOCK_RAW    = (3U)   /**< @brief Raw socket.@n
                         * Raw sockets allow an application to have direct access to
                         * lower-level communication protocols.
                         * Raw sockets are intended to take advantage of some protocol feature
                         * that is not directly accessible through a normal interface,
                         * or to build new protocols on top of existing low-level protocols.@n
                         * It can be enabled by the @ref FNET_CFG_RAW option.
                         */
} socket_type_t;


/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
int16_t socket(int domain, int type, int protocol);
err_return_t bind(int s, const struct sockaddr *name, uint16_t namelen);
int32_t connect(int s, const struct sockaddr *name, socklen_t namelen);
err_return_t listen(int s, uint16_t backlog);
int8_t accept(int  s, struct sockaddr *addr, uint16_t *addrlen);
int32_t send(int s, const void *buf, size_t len, int8_t flags);
int32_t sendto(int s, const void *data, size_t size, int8_t flags, const struct sockaddr *to, socklen_t tolen);
int32_t recv(int s, void *data, size_t len, int8_t flags);
int32_t recvfrom(int s, void *mem, size_t len, int8_t flags, struct sockaddr *from, socklen_t *from_len);
int8_t close(int16_t s);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SS_SOCKET_H */
