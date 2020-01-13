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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
  
#include "sys/socket.h"  
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/errno.h"
#include "sys/select.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "assert.h"
#include "ssDevMan.h"
#include "ssSocket.h" 
#include "ssLogging.h"
#include "fifo.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define MODEM_SOCKET      1
#define WIFI_SOCKET       2

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/************************************************************************
*    Structure per socket.
*************************************************************************/
struct socket_t {
  /* Socket state */
  socket_state_t state;
  /* Data that was left from the previous read */
  FifoHandle_t rxfifo;
  /* Offset in the data that was left from the previous read */
  uint16_t lastoffset;
#ifdef  SOCKETS_DEBUG
  /* Number of times data was received */
  int16_t rcevent;
  /* Number of times data was sent */
  uint16_t sendevent;
  /* Socket flags */
  uint16_t flags;
  /* Last error that occured on this socket */
  int8_t err;
#endif /* SOCKETS_DEBUG */
  /* Status of socket */
  uint8_t taken;
  uint8_t protocol;
  socket_type_t type;
  struct netif_t *interface;
};

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
/* The global array of available sockets */
struct socket_t sockets[NUM_SOCKETS];

extern int errno;

/*------------------------- PRIVATE VARIABLES --------------------------------*/


/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
static struct addrinfo *allocaddrinfo(void);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

static struct socket_t *get_socket(int16_t s)
{
  struct socket_t *sock;

  if ((s < 0) || (s >= NUM_SOCKETS)) {
    return NULL;
  }

  sock = &sockets[s];

  return sock;
}

/* Create a socket and return the descriptor to the app */
int16_t socket(int domain, int type, int protocol)
{
  int8_t i;
  netif_t *net_dev;
  
  net_dev = get_device();
  i = net_dev->socket_open(domain, type, protocol);
  
  if(i > -1 && !sockets[i].taken) 
  {
    ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket created id=%d (domain=%d, type=%d, protocol=%d)", i, domain, type, protocol);
    sockets[i].state      = SS_UNCONNECTED;
    sockets[i].rxfifo     = fifo_create(NULL, SOCKBUF_SIZE);;
    sockets[i].lastoffset = 0;
#ifdef  SOCKETS_DEBUG
    sockets[i].rcevent    = 0;
    sockets[i].sendevent  = 0;
    sockets[i].flags      = 0;
    sockets[i].err        = 0;
#endif /* SOCKETS_DEBUG */
    sockets[i].taken      = 1;
    sockets[i].protocol   = protocol;
    sockets[i].type       = type;
    sockets[i].interface  = net_dev;
    return i;
  }
  
  return i;
}

int32_t connect(int s, const struct sockaddr *name, socklen_t namelen)
{
  uint8_t result = -1;
  struct socket_t *sock;

  sock = get_socket(s);
  if (!sock)
  {
    return result;
  }

  /* TODO - not  in POSIX style  but makes working with ublox modems easier */
  if (sock->state == SS_CONNECTED)
  {
    ssLoggingPrint(ESsLoggingLevel_Error, 0, "socket %d is already connected", s);
    return result;
  }

  result = sock->interface->socket_connect(sock->interface, s, name, namelen);

  if (result == 0)
  {
    sock->state = SS_CONNECTED;
  }
  return result;
}

int32_t sendto(int s, const void *data, size_t size, int8_t flags, const struct sockaddr *to, socklen_t tolen)
{
  struct socket_t *sock;
  int32_t     result = -1;

  sock = get_socket(s);
  if (!sock)
    return -1;

  result = sock->interface->socket_sendto(sock->interface, s, (char *)data, size, 0, (const struct sockaddr_in *)to, tolen);
  //ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, data, size, "socket sendto s=%d to=0x%x:%d =>", s, ((const struct sockaddr_in *)to)->sin_addr.s_addr, ((const struct sockaddr_in *)to)->sin_port);

#ifdef SOCKETS_DEBUG
  if (result)
  {
    //sock->sendevent += 1;
  }
#endif
  return result;
}

int32_t send(int s, const void *data, size_t size, int8_t flags)
{
  struct socket_t *sock;
  int8_t     result = -1;

  sock = get_socket(s);
  if (!sock)
    return result;

  if (sock->state == SS_CONNECTED)
  {
      result = sock->interface->socket_send(sock->interface, s, data, size, 0);
  }
#ifdef SOCKETS_DEBUG
  if (result)
  {
    sock->sendevent += 1;
  }
#endif

  return result;
}

int32_t recvfrom(int s, void *mem, size_t len, int8_t flags, struct sockaddr *from, socklen_t *from_len)
{
  struct socket_t   *sock;
  int32_t           recv_length = -1;
  char buf[SOCKBUF_SIZE] = {0};
  uint16_t copylen = 0;

  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }

  copylen = fifo_length(sock->rxfifo);

  if (copylen)
  {
    /* There is data left from the last recv operation */
    fifo_read(sock->rxfifo, mem, copylen, 0);
    recv_length = copylen;
  }
  else
  {
     recv_length = sock->interface->socket_recvfrom(sock->interface, s, buf, len, 0, (struct sockaddr_in *)from, from_len);
   
    if(recv_length > 0)
    {
      ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, mem, recv_length, "socket recvfrom s=%d from=0x%x:d =>", s, ((const struct sockaddr_in *)from)->sin_addr.s_addr, ((const struct sockaddr_in *)from)->sin_port);
      memcpy(mem, buf, recv_length);
      if (recv_length > len)
      {
        /* If all data doesn't fit into provided buffer, we'll copy what we can and the rest we'll save for later */
        fifo_write(sock->rxfifo, buf+len, recv_length-len, 0);
      }

#ifdef SOCKETS_DEBUG
    sock->rcevent += 1;
#endif

    }  
  }
  
  return recv_length;
}

int32_t recv(int s, void *mem, size_t len, int8_t flags)
{
  struct socket_t   *sock;
  int32_t           length = -1;

  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }

  length = sock->interface->socket_recv(sock->interface, s, mem, len, 0);

#ifdef SOCKETS_DEBUG
  if (length > 0)
  {
    sock->rcevent += 1;
  }
#endif

  return length;
}

int8_t close (int16_t s)
{
  struct socket_t *sock;
  
  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }
  else if (sock->state == SS_CONNECTED)
  {
    /* Higher libs will sometimes close the socket after it is connected */
    /* Ublox modems can't tell the difference so we won't close the socket in that case */
    return 0;
  }
  else
  {
    sock->interface->socket_close(sock->interface, s);
    fifo_destroy(sock->rxfifo);
    sock->rxfifo     = NULL;
    sock->lastoffset = 0;
    sock->taken      = 0;
    sock->state      = SS_UNCONNECTED;
    return 0;
  }
}

/* @TODO: not finished - DO NOT USE! */
struct hostent *gethostbyname(const char *hostname)
{
  netif_t *net_dev;
  uint32_t ipaddr;
  struct hostent *data;
  
  if(!hostname)
  {
    return NULL;
  }
  
  data = pvPortMalloc(sizeof(struct addrinfo));
  if(data)
  {
    struct in_addr addr;
    const char *p;
    
    /* Check for all-numeric hostname with no trailing dot. */
    if (isdigit(hostname[0])) 
    {
      p = hostname;
      while (*p && (isdigit(*p) || *p == '.'))
      {
        p++;
      }
      
      if (!*p && p[-1] != '.') 
      {
        /* Looks like an IP address; convert it. */
        if (inet_aton(hostname, &addr) == 0) 
        {
          /* invalid ip address */
          return NULL;
        }
        /* @TODO: fill hostent */
        return data;
      }
    }
    
    net_dev = get_device();
    net_dev->gethostbyname(hostname, &ipaddr);
    /* @TODO: fill hostent */
  }
  
  return data;
}

/* Merkat -- still working on it */
int getaddrinfo(const char *node, const char *service,
       const struct addrinfo *hints, struct addrinfo **res)
{
  netif_t *net_dev = NULL;
  struct in_addr addr;
  struct addrinfo *ai = NULL;
  struct sockaddr_in sin = {0};
  bool is_numeric = false;
  bool is_found = false;


  if (res == NULL)
  {
    return -1;
  }
  *res = NULL;

  if ((node == NULL) && (service == NULL)) 
  {
    return -1;
  }
 
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;

  /* Check for all-numeric hostname with no trailing dot.
   * No need to query network device in that case, just copy to output structure.
   */
  if (isdigit(node[0])) 
  {
    const char *p = node;
    while (*p && (isdigit(*p) || *p == '.'))
    {
      p++;
    }
    
    if (!*p && p[-1] != '.') 
    {
      /* Looks like an IP address; convert it. */
      if (inet_aton(node, &addr) != 0) 
      {
        sin.sin_addr = addr;
        is_numeric = true;
        is_found = true;
      }
    }
  }
  
  if((!is_found) && (!is_numeric))
  {
    /* try network device */
    net_dev = get_device();
    if(net_dev->gethostbyname(node, &addr.s_addr) == 0)
    {
      sin.sin_addr = addr;
      is_found = true;
    }
  }
    
  if(is_found)
  {
    ai = allocaddrinfo();
    if(ai)
    {
      ai->ai_family = AF_INET;
      if (hints != NULL) 
      {
        /* copy socktype & protocol from hints if specified */
        ai->ai_socktype = hints->ai_socktype;
        ai->ai_protocol = hints->ai_socktype; /* Merkat also waakama TODO */
      }

      if (service)
      {
        sin.sin_port = htons(atoi(service));
      }
      ai->ai_addr->sa_family = AF_INET;
      ai->ai_addrlen = sizeof(struct sockaddr);   // Merkat temp -DBG
      if(is_numeric)
      {
        strcpy(ai->ai_addr->sa_data, node);
      }
      else
      {
        if(inet_ntop(AF_INET, &addr, ai->ai_addr->sa_data, sizeof(ai->ai_addr->sa_data)) != ai->ai_addr->sa_data)
        {
          freeaddrinfo(ai);
          ai = NULL;
        }
        else
        {
          memcpy(ai->ai_addr->sa_data, ((struct sockaddr *) &sin)->sa_data, sizeof((ai->ai_addr->sa_data)));
        }
      }
      memcpy(ai->ai_addr, &sin, sizeof(sin));
    }
  }

  *res = ai;

  return 0;
}


void freeaddrinfo(struct addrinfo *res)
{
  vPortFree(res);
}


/* Convert string to integer */
/*
static int32_t atoi(const char *str)
{
  int res = 0;
  int i = 0;

  for(; str[i] != '\0'; ++i)
  {
    res = res*10 + str[i] - '0';
  }
  return res;
}
*/

/* dots and numbers string to unsigned long */
uint32_t inet_addr(const char *cp)
{
  struct in_addr val;
  if (inet_aton(cp, &val))
  {
    return val.s_addr;
  }
  return NULL;

}

/* Not supported for now -- TODO */
int select (int __nfds, fd_set *restrict __readfds, fd_set *restrict __writefds, fd_set *restrict __exceptfds, struct timeval *restrict __timeout)
{
  return 1;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
static struct addrinfo *allocaddrinfo()
{
  void *res = NULL;
  struct addrinfo *ai = NULL;
  struct sockaddr *sa = NULL;
  
  res = pvPortMalloc(sizeof(struct addrinfo) + sizeof(struct sockaddr));
  if(res)
  {
    memset(res, 0, sizeof(struct addrinfo) + sizeof(struct sockaddr));
    ai = (struct addrinfo *)res;
    sa = (struct sockaddr *)((uint8_t *)res + sizeof(struct addrinfo));
    
    ai->ai_addr = sa;
    /* Canonname not supported */
    ai->ai_canonname = NULL;
    /* Only single address per host is supported */
    ai->ai_next = NULL;
  }
 
  return ai;
}
