#ifndef _NGX_MYTCP_H_INCLUDED_
#define _NGX_MYTCP_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

#if (NGX_MYTCP_SSL)
#include <ngx_mytcp_ssl_module.h>
#endif


typedef struct ngx_mytcp_session_s  ngx_mytcp_session_t;


//#include <ngx_mytcp_upmytcp.h>
//#include <ngx_mytcp_upmytcp_round_robin.h>


typedef struct {
    void                  **main_conf;
    void                  **srv_conf;
} ngx_mytcp_conf_ctx_t;


typedef struct {
    union {
        struct sockaddr     sockaddr;
        struct sockaddr_in  sockaddr_in;
#if (NGX_HAVE_INET6)
        struct sockaddr_in6 sockaddr_in6;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
        struct sockaddr_un  sockaddr_un;
#endif
        u_char              sockaddr_data[NGX_SOCKADDRLEN];
    } u;

    socklen_t               socklen;

    /* server ctx */
    ngx_mytcp_conf_ctx_t  *ctx;

    unsigned                bind:1;
    unsigned                wildcard:1;
#if (NGX_MYTCP_SSL)
    unsigned                ssl:1;
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned                ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned                reuseport:1;
#endif
    unsigned                so_keepalive:2;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                     tcp_keepidle;
    int                     tcp_keepintvl;
    int                     tcp_keepcnt;
#endif
    int                     backlog;
    int                     type;
} ngx_mytcp_listen_t;


typedef struct {
    ngx_mytcp_conf_ctx_t  *ctx;
    ngx_str_t               addr_text;
#if (NGX_MYTCP_SSL)
    ngx_uint_t              ssl;    /* unsigned   ssl:1; */
#endif
} ngx_mytcp_addr_conf_t;

typedef struct {
    in_addr_t               addr;
    ngx_mytcp_addr_conf_t  conf;
} ngx_mytcp_in_addr_t;


#if (NGX_HAVE_INET6)

typedef struct {
    struct in6_addr         addr6;
    ngx_mytcp_addr_conf_t  conf;
} ngx_mytcp_in6_addr_t;

#endif


typedef struct {
    /* ngx_mytcp_in_addr_t or ngx_mytcp_in6_addr_t */
    void                   *addrs;
    ngx_uint_t              naddrs;
} ngx_mytcp_port_t;


typedef struct {
    int                     family;
    int                     type;
    in_port_t               port;
    ngx_array_t             addrs;       /* array of ngx_mytcp_conf_addr_t */
} ngx_mytcp_conf_port_t;


typedef struct {
    ngx_mytcp_listen_t     opt;
} ngx_mytcp_conf_addr_t;


typedef ngx_int_t (*ngx_mytcp_access_pt)(ngx_mytcp_session_t *s);


typedef struct {
    ngx_array_t             servers;     /* ngx_mytcp_core_srv_conf_t */
    ngx_array_t             listen;      /* ngx_mytcp_listen_t */
    ngx_mytcp_access_pt    limit_conn_handler;
    ngx_mytcp_access_pt    access_handler;
} ngx_mytcp_core_main_conf_t;


//typedef void (*ngx_mytcp_handler_pt)(ngx_mytcp_session_t *s);


typedef struct {
//    ngx_mytcp_handler_pt   handler;
    ngx_mytcp_conf_ctx_t  *ctx;
    u_char                 *file_name;
    ngx_int_t               line;
    ngx_log_t              *error_log;
    ngx_flag_t              tcp_nodelay;
} ngx_mytcp_core_srv_conf_t;


struct ngx_mytcp_session_s {
    uint32_t                signature;         /* "STRM" */

    ngx_connection_t       *connection;

    off_t                   received;

    ngx_log_handler_pt      log_handler;

    void                  **ctx;
    void                  **main_conf;
    void                  **srv_conf;
};


typedef struct {
    ngx_int_t             (*postconfiguration)(ngx_conf_t *cf);

    void                 *(*create_main_conf)(ngx_conf_t *cf);
    char                 *(*init_main_conf)(ngx_conf_t *cf, void *conf);

    void                 *(*create_srv_conf)(ngx_conf_t *cf);
    char                 *(*merge_srv_conf)(ngx_conf_t *cf, void *prev,
                                            void *conf);
} ngx_mytcp_module_t;


#define NGX_MYTCP_MODULE       0x4d544350     /* "MTCP" */

#define NGX_MYTCP_MAIN_CONF    0x02000000
#define NGX_MYTCP_SRV_CONF     0x04000000
#define NGX_MYTCP_UPS_CONF     0x08000000


#define NGX_MYTCP_MAIN_CONF_OFFSET  offsetof(ngx_mytcp_conf_ctx_t, main_conf)
#define NGX_MYTCP_SRV_CONF_OFFSET   offsetof(ngx_mytcp_conf_ctx_t, srv_conf)


#define ngx_mytcp_get_module_ctx(s, module)   (s)->ctx[module.ctx_index]
#define ngx_mytcp_set_ctx(s, c, module)       s->ctx[module.ctx_index] = c;
#define ngx_mytcp_delete_ctx(s, module)       s->ctx[module.ctx_index] = NULL;


#define ngx_mytcp_get_module_main_conf(s, module)                             \
    (s)->main_conf[module.ctx_index]
#define ngx_mytcp_get_module_srv_conf(s, module)                              \
    (s)->srv_conf[module.ctx_index]

#define ngx_mytcp_conf_get_module_main_conf(cf, module)                       \
    ((ngx_mytcp_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_mytcp_conf_get_module_srv_conf(cf, module)                        \
    ((ngx_mytcp_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]

#define ngx_mytcp_cycle_get_module_main_conf(cycle, module)                   \
    (cycle->conf_ctx[ngx_mytcp_module.index] ?                                \
        ((ngx_mytcp_conf_ctx_t *) cycle->conf_ctx[ngx_mytcp_module.index])   \
            ->main_conf[module.ctx_index]:                                     \
        NULL)


void ngx_mytcp_init_connection(ngx_connection_t *c);
void ngx_mytcp_close_connection(ngx_connection_t *c);

extern ngx_module_t  ngx_mytcp_module;
extern ngx_uint_t    ngx_mytcp_max_module;
extern ngx_module_t  ngx_mytcp_core_module;


#endif /* _NGX_MYTCP_H_INCLUDED_ */
