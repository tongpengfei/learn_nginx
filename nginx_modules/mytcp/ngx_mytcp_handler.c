
/*
 * Copyright (C) Roman Arutyunyan
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_mytcp.h>


static u_char *ngx_mytcp_log_error(ngx_log_t *log, u_char *buf, size_t len);
static void ngx_mytcp_init_session(ngx_connection_t *c);

#if (NGX_MYTCP_SSL)
static void ngx_mytcp_ssl_init_connection(ngx_ssl_t *ssl, ngx_connection_t *c);
static void ngx_mytcp_ssl_handshake_handler(ngx_connection_t *c);
#endif

static void ngx_mytcp_tcp_recv_handler(ngx_event_t *ev);
static void ngx_mytcp_tcp_send_handler(ngx_event_t *ev);

void
ngx_mytcp_init_connection(ngx_connection_t *c)
{
    int                           tcp_nodelay;
    u_char                        text[NGX_SOCKADDR_STRLEN];
    size_t                        len;
    ngx_int_t                     rc;
    ngx_uint_t                    i;
    struct sockaddr              *sa;
    ngx_mytcp_port_t            *port;
    struct sockaddr_in           *sin;
    ngx_mytcp_in_addr_t         *addr;
    ngx_mytcp_session_t         *s;
    ngx_mytcp_addr_conf_t       *addr_conf;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6          *sin6;
    ngx_mytcp_in6_addr_t        *addr6;
#endif
    ngx_mytcp_core_srv_conf_t   *cscf;
    ngx_mytcp_core_main_conf_t  *cmcf;

    /* find the server configuration for the address:port */

    port = c->listening->servers;

    if (port->naddrs > 1) {

        /*
         * There are several addresses on this port and one of them
         * is the "*:port" wildcard so getsockname() is needed to determine
         * the server address.
         *
         * AcceptEx() and recvmsg() already gave this address.
         */

        if (ngx_connection_local_sockaddr(c, NULL, 0) != NGX_OK) {
            ngx_mytcp_close_connection(c);
            return;
        }

        sa = c->local_sockaddr;

        switch (sa->sa_family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            sin6 = (struct sockaddr_in6 *) sa;

            addr6 = port->addrs;

            /* the last address is "*" */

            for (i = 0; i < port->naddrs - 1; i++) {
                if (ngx_memcmp(&addr6[i].addr6, &sin6->sin6_addr, 16) == 0) {
                    break;
                }
            }

            addr_conf = &addr6[i].conf;

            break;
#endif

        default: /* AF_INET */
            sin = (struct sockaddr_in *) sa;

            addr = port->addrs;

            /* the last address is "*" */

            for (i = 0; i < port->naddrs - 1; i++) {
                if (addr[i].addr == sin->sin_addr.s_addr) {
                    break;
                }
            }

            addr_conf = &addr[i].conf;

            break;
        }

    } else {
        switch (c->local_sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            addr6 = port->addrs;
            addr_conf = &addr6[0].conf;
            break;
#endif

        default: /* AF_INET */
            addr = port->addrs;
            addr_conf = &addr[0].conf;
            break;
        }
    }

    s = ngx_pcalloc(c->pool, sizeof(ngx_mytcp_session_t));
    if (s == NULL) {
        ngx_mytcp_close_connection(c);
        return;
    }

    s->signature = NGX_MYTCP_MODULE;
    s->main_conf = addr_conf->ctx->main_conf;
    s->srv_conf = addr_conf->ctx->srv_conf;

    s->connection = c;
    c->data = s;

    cscf = ngx_mytcp_get_module_srv_conf(s, ngx_mytcp_core_module);

    ngx_set_connection_log(c, cscf->error_log);

    len = ngx_sock_ntop(c->sockaddr, c->socklen, text, NGX_SOCKADDR_STRLEN, 1);

    ngx_log_error(NGX_LOG_INFO, c->log, 0, "*%uA %sclient %*s connected to %V",
                  c->number, c->type == SOCK_DGRAM ? "udp " : "",
                  len, text, &addr_conf->addr_text);

    c->log->connection = c->number;
    c->log->handler = ngx_mytcp_log_error;
    c->log->data = s;
    c->log->action = "initializing connection";
    c->log_error = NGX_ERROR_INFO;

    cmcf = ngx_mytcp_get_module_main_conf(s, ngx_mytcp_core_module);

    if (cmcf->limit_conn_handler) {
        rc = cmcf->limit_conn_handler(s);

        if (rc != NGX_DECLINED) {
            ngx_mytcp_close_connection(c);
            return;
        }
    }

    if (cmcf->access_handler) {
        rc = cmcf->access_handler(s);

        if (rc != NGX_OK && rc != NGX_DECLINED) {
            ngx_mytcp_close_connection(c);
            return;
        }
    }

    if (c->type == SOCK_STREAM
        && cscf->tcp_nodelay
        && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "tcp_nodelay");

        tcp_nodelay = 1;

        if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                       (const void *) &tcp_nodelay, sizeof(int)) == -1)
        {
            ngx_connection_error(c, ngx_socket_errno,
                                 "setsockopt(TCP_NODELAY) failed");
            ngx_mytcp_close_connection(c);
            return;
        }

        c->tcp_nodelay = NGX_TCP_NODELAY_SET;
    }


#if (NGX_MYTCP_SSL)
    {
    ngx_mytcp_ssl_conf_t  *sslcf;

    sslcf = ngx_mytcp_get_module_srv_conf(s, ngx_mytcp_ssl_module);

    if (addr_conf->ssl) {
        c->log->action = "SSL handshaking";

        if (sslcf->ssl.ctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, c->log, 0,
                          "no \"ssl_certificate\" is defined "
                          "in server listening on SSL port");
            ngx_mytcp_close_connection(c);
            return;
        }

        ngx_mytcp_ssl_init_connection(&sslcf->ssl, c);
        return;
    }
    }
#endif

    ngx_mytcp_init_session(c);
}


static void ngx_mytcp_init_session(ngx_connection_t *c)
{
    ngx_mytcp_session_t        *s;
    ngx_mytcp_core_srv_conf_t  *cscf;

    s = c->data;
    c->log->action = "handling client connection";

    cscf = ngx_mytcp_get_module_srv_conf(s, ngx_mytcp_core_module);

    s->ctx = ngx_pcalloc(c->pool, sizeof(void *) * ngx_mytcp_max_module);
    if (s->ctx == NULL) {
        ngx_mytcp_close_connection(c);
        return;
    }

    ngx_buf_t* b = ngx_create_temp_buf(c->pool, 10);
    if (b == NULL) {
        ngx_http_close_connection(c);
        return;
    }

    c->buffer = b;

    c->write->handler = ngx_mytcp_tcp_send_handler;
    c->read->handler = ngx_mytcp_tcp_recv_handler;

    int flags = 0;
    if( ngx_handle_read_event(c->read, flags) != NGX_OK){
        ngx_mytcp_close_connection(c);
    }
}


#if (NGX_MYTCP_SSL)

static void
ngx_mytcp_ssl_init_connection(ngx_ssl_t *ssl, ngx_connection_t *c)
{
    ngx_mytcp_session_t   *s;
    ngx_mytcp_ssl_conf_t  *sslcf;

    if (ngx_ssl_create_connection(ssl, c, 0) == NGX_ERROR) {
        ngx_mytcp_close_connection(c);
        return;
    }

    if (ngx_ssl_handshake(c) == NGX_AGAIN) {

        s = c->data;

        sslcf = ngx_mytcp_get_module_srv_conf(s, ngx_mytcp_ssl_module);

        ngx_add_timer(c->read, sslcf->handshake_timeout);

        c->ssl->handler = ngx_mytcp_ssl_handshake_handler;

        return;
    }

    ngx_mytcp_ssl_handshake_handler(c);
}


static void
ngx_mytcp_ssl_handshake_handler(ngx_connection_t *c)
{
    if (!c->ssl->handshaked) {
        ngx_mytcp_close_connection(c);
        return;
    }

    if (c->read->timer_set) {
        ngx_del_timer(c->read);
    }

    ngx_mytcp_init_session(c);
}

#endif


void
ngx_mytcp_close_connection(ngx_connection_t *c)
{
    ngx_pool_t  *pool;

    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0,
                   "close stream connection: %d", c->fd);

#if (NGX_MYTCP_SSL)

    if (c->ssl) {
        if (ngx_ssl_shutdown(c) == NGX_AGAIN) {
            c->ssl->handler = ngx_mytcp_close_connection;
            return;
        }
    }

#endif

#if (NGX_STAT_STUB)
    (void) ngx_atomic_fetch_add(ngx_stat_active, -1);
#endif

    pool = c->pool;

    ngx_close_connection(c);

    ngx_destroy_pool(pool);
}


static u_char *
ngx_mytcp_log_error(ngx_log_t *log, u_char *buf, size_t len)
{
    u_char                *p;
    ngx_mytcp_session_t  *s;

    if (log->action) {
        p = ngx_snprintf(buf, len, " while %s", log->action);
        len -= p - buf;
        buf = p;
    }

    s = log->data;

    p = ngx_snprintf(buf, len, ", %sclient: %V, server: %V",
                     s->connection->type == SOCK_DGRAM ? "udp " : "",
                     &s->connection->addr_text,
                     &s->connection->listening->addr_text);
    len -= p - buf;
    buf = p;

    if (s->log_handler) {
        p = s->log_handler(log, buf, len);
    }

    return p;
}

static void ngx_mytcp_tcp_recv_handler(ngx_event_t *ev){
    ngx_connection_t *c;
    ngx_mytcp_session_t *s;
    ngx_buf_t *b;

    c = ev->data;
    s = c->data;

    if( !ev->ready ){
        return;
    }

    b = c->buffer;

    size_t blen = 0;
    ssize_t n = 0;

    size_t nrecv = 0;
    do{
        blen = b->end - b->last;
        n = c->recv(c, b->last, blen);

        if( n > 0 ){
            nrecv += n;
            b->last += n;

            if( n < blen ){
                break;
            }
        }

        if( n == NGX_AGAIN ){
            if (ngx_handle_read_event(ev, 0) != NGX_OK) {
                ngx_mytcp_close_connection(c);
                return;
            }
            break;
        }

        if (n == NGX_ERROR) {
            ngx_mytcp_close_connection(c);
            return;
        }

        if( n == 0 ) {
            ngx_log_error(NGX_LOG_INFO, c->log, 0, "client closed connection");
            ngx_mytcp_close_connection(c);
            return;
        }

    }while(b->last != b->end);

    if( nrecv <= 0 ){
        return;
    }

    if( b->last == b->end ){
        ngx_post_event(ev, &ngx_posted_events);
    }

    //print data
    printf("recv %s\n", b->start);
    b->last = b->start; 
    b->pos = b->last;
}

static void ngx_mytcp_tcp_send_handler(ngx_event_t *ev){
    int a;
    a = 10;
}
