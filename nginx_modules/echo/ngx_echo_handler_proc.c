#include "ngx_echo_handler_proc.h"

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
	ngx_str_t uri;
	ngx_http_handler_pt handler;
} echo_handler;

static ngx_int_t handler_default(ngx_http_request_t *r);

static ngx_int_t handler_ver(ngx_http_request_t *r);
static ngx_int_t handler_queue(ngx_http_request_t *r);

static echo_handler handlers[] = {
	{ ngx_string("/echo/ver"), handler_ver },
	{ ngx_string("/echo/queue"), handler_queue },
};

static ngx_int_t send_response(ngx_http_request_t* r, ngx_str_t* res){
	ngx_int_t rc = 0;

	ngx_str_t type = ngx_string("text/plain");
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = res->len;
	r->headers_out.content_type = type;

	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		return rc;
	}

	ngx_buf_t* b = ngx_create_temp_buf(r->pool, res->len);
	if( NULL == b ){
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	ngx_memcpy(b->pos, res->data, res->len);
	b->last = b->pos + res->len;
	b->last_buf = 1;

	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	return ngx_http_output_filter(r, &out);
}

static ngx_int_t handler_default(ngx_http_request_t *r){

	ngx_str_t res = ngx_string("echo cant find uri handler!\n");
	ngx_int_t rc = send_response(r, &res);
	return rc;
}

static ngx_int_t handler_ver(ngx_http_request_t *r){

	ngx_str_t res = ngx_string("echo 1.0.0\n");
	ngx_int_t rc = send_response(r, &res);
	return rc;
}

static ngx_int_t handler_queue(ngx_http_request_t *r){

	ngx_str_t res = ngx_string("echo test queue\n");
	ngx_int_t rc = send_response(r, &res);
	return rc;
}

ngx_int_t ngx_echo_handler_proc(ngx_http_request_t *r){

	static const ngx_int_t nhandler = sizeof(handlers)/sizeof(handlers[0]);
	(void)nhandler;

	ngx_str_t* puri = &(r->uri);

	ngx_int_t rc = 0;
	echo_handler* eh = NULL;
	for( ngx_int_t i=0; i<nhandler; ++i ){
		eh = handlers + i;

		if (!ngx_strncmp(puri->data, eh->uri.data, eh->uri.len)) {
			rc = eh->handler(r);
			return rc;
		}
	}

	rc = handler_default(r);
	return rc;
}
