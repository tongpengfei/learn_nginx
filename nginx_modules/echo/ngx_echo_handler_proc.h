#ifndef __ngx_echo_handler_proc_h__
#define __ngx_echo_handler_proc_h__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

ngx_int_t ngx_echo_handler_proc(ngx_http_request_t *r);

#endif//__ngx_echo_handler_types_h__