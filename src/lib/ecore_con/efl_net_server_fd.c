#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#include <fcntl.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_EVIL
# include <Evil.h>
#endif

#ifdef HAVE_SYSTEMD
# include <systemd/sd-daemon.h>
#endif

#define MY_CLASS EFL_NET_SERVER_FD_CLASS

typedef struct _Efl_Net_Server_Fd_Data
{
   Eina_Stringshare *address;
   int family;
   unsigned int clients_count;
   unsigned int clients_limit;
   Eina_Bool clients_reject_excess;
   Eina_Bool serving;
   Eina_Bool close_on_exec;
   Eina_Bool reuse_address;
   Eina_Bool reuse_port;
} Efl_Net_Server_Fd_Data;

static SOCKET
efl_net_accept4(SOCKET fd, struct sockaddr *addr, socklen_t *addrlen, Eina_Bool close_on_exec)
{
#ifdef HAVE_ACCEPT4
   int flags = 0;
   if (close_on_exec) flags |= SOCK_CLOEXEC;
   return accept4(fd, addr, addrlen, flags);
#else
   SOCKET client = accept(fd, addr, addrlen);
   if (client == INVALID_SOCKET) return client;

#ifdef FD_CLOEXEC
   if (close_on_exec)
     {
        if (fcntl(client, F_SETFD, FD_CLOEXEC) < 0)
          {
             int errno_bkp = errno;
             ERR("fcntl(%d, F_SETFD, FD_CLOEXEC): %s", client, strerror(errno));
             closesocket(client);
             errno = errno_bkp;
             return INVALID_SOCKET;
          }
     }
#endif

   return client;
#endif
}

static void
_efl_net_server_fd_event_read(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_net_server_fd_process_incoming_data(event->object);
}

static void
_efl_net_server_fd_event_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *o = event->object;
   Eina_Error err = EBADF;

   efl_net_server_serving_set(o, EINA_FALSE);
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_ERROR, &err);
}

EOLIAN static Efl_Object *
_efl_net_server_fd_efl_object_finalize(Eo *o, Efl_Net_Server_Fd_Data *pd EINA_UNUSED)
{
   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   // TODO: only register "read" if "can_read" is being monitored?
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_READ, _efl_net_server_fd_event_read, NULL);
   efl_event_callback_add(o, EFL_LOOP_FD_EVENT_ERROR, _efl_net_server_fd_event_error, NULL);
   return o;
}

EOLIAN static Efl_Object *
_efl_net_server_fd_efl_object_constructor(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   pd->family = AF_UNSPEC;
   return efl_constructor(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_net_server_fd_efl_object_destructor(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   SOCKET fd = efl_loop_fd_get(o);

   if (fd != INVALID_SOCKET)
     {
        efl_loop_fd_set(o, INVALID_SOCKET);
        closesocket(fd);
     }

   efl_destructor(efl_super(o, MY_CLASS));

   eina_stringshare_replace(&pd->address, NULL);
}

EOLIAN static void
_efl_net_server_fd_efl_loop_fd_fd_set(Eo *o, Efl_Net_Server_Fd_Data *pd, int fd)
{
   efl_loop_fd_set(efl_super(o, MY_CLASS), fd);

   if (fd != INVALID_SOCKET)
     {
        /* apply postponed values */
        efl_net_server_fd_close_on_exec_set(o, pd->close_on_exec);
        efl_net_server_fd_reuse_address_set(o, pd->reuse_address);
        efl_net_server_fd_reuse_port_set(o, pd->reuse_port);

        if (pd->family == AF_UNSPEC)
          {
             ERR("efl_loop_fd_set() must be called after efl_net_server_fd_family_set()");
             return;
          }
     }
   else
     {
        efl_net_server_address_set(o, NULL);
     }
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_address_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, const char *address)
{
   eina_stringshare_replace(&pd->address, address);
}

EOLIAN static const char *
_efl_net_server_fd_efl_net_server_address_get(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->address;
}

EOLIAN static unsigned int
_efl_net_server_fd_efl_net_server_clients_count_get(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->clients_count;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_count_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int count)
{
   pd->clients_count = count;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_limit_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int limit, Eina_Bool reject_excess)
{
   pd->clients_limit = limit;
   pd->clients_reject_excess = reject_excess;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_clients_limit_get(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, unsigned int *limit, Eina_Bool *reject_excess)
{
   if (limit) *limit = pd->clients_limit;
   if (reject_excess) *reject_excess = pd->clients_reject_excess;
}

EOLIAN static void
_efl_net_server_fd_efl_net_server_serving_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, Eina_Bool serving)
{
   if (pd->serving == serving) return;
   pd->serving = serving;
   if (serving)
     efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVING, NULL);
}

EOLIAN static Eina_Error
_efl_net_server_fd_efl_net_server_serve(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd EINA_UNUSED, const char *address)
{
   DBG("address=%s", address);
   return 0;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_efl_net_server_serving_get(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->serving;
}

EOLIAN static Eina_Error
_efl_net_server_fd_socket_activate(Eo *o, Efl_Net_Server_Fd_Data *pd EINA_UNUSED, const char *address)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_loop_fd_get(o) != INVALID_SOCKET, EALREADY);
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);

#ifndef HAVE_SYSTEMD
   DBG("systemd support is disabled");
   return ENOENT;
#else
   if (!sd_fd_max)
     {
        DBG("This service was not socket-activated, no $LISTEN_FDS");
        return ENOENT;
     }
   else if (sd_fd_index >= sd_fd_max)
     {
        WRN("No more systemd sockets available. Configuration mismatch?");
        return ENOENT;
     }
   else
     {
        SOCKET fd = SD_LISTEN_FDS_START + sd_fd_index;
        int family;
        socklen_t len = sizeof(family);

        if (getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &family, &len) != 0)
          {
             WRN("socket %d failed to return family: %s", fd, eina_error_msg_get(efl_net_socket_error_get()));
             return EINVAL;
          }

        sd_fd_index++;
        efl_net_server_fd_family_set(o, family);
        efl_loop_fd_set(o, fd);
        if (efl_loop_fd_get(o) == INVALID_SOCKET)
          {
             sd_fd_index--;
             WRN("socket %d could not be used by %p (%s)",
                 fd, o, efl_class_name_get(efl_class_get(o)));
             return EINVAL;
          }

        /* by default they all come with close_on_exec set
         * and we must apply our local conf.
         */
        efl_net_server_fd_close_on_exec_set(o, pd->close_on_exec);
        return 0;
     }
#endif
}

EOLIAN static Eina_Bool
_efl_net_server_fd_close_on_exec_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool close_on_exec)
{
#ifdef FD_CLOEXEC
   int flags, fd;
   Eina_Bool old = pd->close_on_exec;
#endif

   pd->close_on_exec = close_on_exec;

#ifdef FD_CLOEXEC
   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   flags = fcntl(fd, F_GETFD);
   if (flags < 0)
     {
        ERR("fcntl(%d, F_GETFD): %s", fd, strerror(errno));
        pd->close_on_exec = old;
        return EINA_FALSE;
     }
   if (close_on_exec)
     flags |= FD_CLOEXEC;
   else
     flags &= (~FD_CLOEXEC);
   if (fcntl(fd, F_SETFD, flags) < 0)
     {
        ERR("fcntl(%d, F_SETFD, %#x): %s", fd, flags, strerror(errno));
        pd->close_on_exec = old;
        return EINA_FALSE;
     }
#endif

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_close_on_exec_get(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
#ifdef FD_CLOEXEC
   int flags, fd;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->close_on_exec;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   flags = fcntl(fd, F_GETFD);
   if (flags < 0)
     {
        ERR("fcntl(%d, F_GETFD): %s", fd, strerror(errno));
        return EINA_FALSE;
     }

   pd->close_on_exec = !!(flags & FD_CLOEXEC); /* sync */
#endif
   return pd->close_on_exec;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_address_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool reuse_address)
{
   int value, fd;
   Eina_Bool old = pd->reuse_address;

   pd->reuse_address = reuse_address;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   value = reuse_address;
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) != 0)
     {
        ERR("setsockopt(%d, SOL_SOCKET, SO_REUSEADDR, %d): %s",
            fd, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->reuse_address = old;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_address_get(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   int value = 0, fd;
   socklen_t valuelen;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->reuse_address;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, &valuelen) != 0)
     {
        ERR("getsockopt(%d, SOL_SOCKET, SO_REUSEADDR): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->reuse_address = !!value; /* sync */
   return pd->reuse_address;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_port_set(Eo *o, Efl_Net_Server_Fd_Data *pd, Eina_Bool reuse_port)
{
#ifdef SO_REUSEPORT
   int value, fd;
   Eina_Bool old = pd->reuse_port;
#endif

   pd->reuse_port = reuse_port;

#ifdef SO_REUSEPORT
   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return EINA_TRUE; /* postpone until fd_set() */

   value = reuse_port;
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) != 0)
     {
        ERR("setsockopt(%d, SOL_SOCKET, SO_REUSEPORT, %d): %s",
            fd, value, eina_error_msg_get(efl_net_socket_error_get()));
        pd->reuse_port = old;
        return EINA_FALSE;
     }
#endif

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_net_server_fd_reuse_port_get(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
#ifdef SO_REUSEPORT
   int value = 0, fd;
   socklen_t valuelen;

   fd = efl_loop_fd_get(o);
   if (fd == INVALID_SOCKET) return pd->reuse_port;

   /* if there is a fd, always query it directly as it may be modified
    * elsewhere by nasty users.
    */
   valuelen = sizeof(value);
   if (getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &value, &valuelen) != 0)
     {
        ERR("getsockopt(%d, SOL_SOCKET, SO_REUSEPORT): %s",
            fd, eina_error_msg_get(efl_net_socket_error_get()));
        return EINA_FALSE;
     }

   pd->reuse_port = !!value; /* sync */
#endif

   return pd->reuse_port;
}

EOLIAN static void
_efl_net_server_fd_family_set(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd, int family)
{
   pd->family = family;
}

EOLIAN static int
_efl_net_server_fd_family_get(Eo *o EINA_UNUSED, Efl_Net_Server_Fd_Data *pd)
{
   return pd->family;
}

EOLIAN static void
_efl_net_server_fd_process_incoming_data(Eo *o, Efl_Net_Server_Fd_Data *pd)
{
   Eina_Bool do_reject = EINA_FALSE;
   struct sockaddr_storage addr;
   SOCKET client, fd;
   socklen_t addrlen;

   if ((pd->clients_limit > 0) && (pd->clients_count >= pd->clients_limit))
     {
        if (!pd->clients_reject_excess)
          {
             // TODO: disconnect 'read' so stops calling?
             return;
          }
        do_reject = EINA_TRUE;
     }

   fd = efl_loop_fd_get(o);

   addrlen = sizeof(addr);
   client = efl_net_accept4(fd, (struct sockaddr *)&addr, &addrlen,
                            efl_net_server_fd_close_on_exec_get(o));
   if (client == INVALID_SOCKET)
     {
        Eina_Error err = efl_net_socket_error_get();
        ERR("accept(%d): %s", fd, eina_error_msg_get(err));
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_ERROR, &err);
        return;
     }

   if (do_reject)
     efl_net_server_fd_client_reject(o, client);
   else
     efl_net_server_fd_client_add(o, client);
}

#include "efl_net_server_fd.eo.c"
