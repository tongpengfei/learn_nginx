#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include "ngx_echo_handler_proc.h"

static char *ngx_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_echo_commands[] = {
    { ngx_string("echo"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
        ngx_echo,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    ngx_null_command
};

static ngx_http_module_t ngx_echo_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL, /* merge location configuration */
};


ngx_module_t ngx_echo_module = {
    NGX_MODULE_V1,
    &ngx_echo_module_ctx,     /* module context */
    ngx_echo_commands,        /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_echo_handler(ngx_http_request_t *r){

	if(!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}

	ngx_int_t rc = ngx_http_discard_request_body(r);
	if( rc != NGX_OK ){
		return rc;
	}

	rc = ngx_echo_handler_proc(r);	
	return rc;
}

static char* ngx_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_echo_handler;

    return NGX_CONF_OK;
}
