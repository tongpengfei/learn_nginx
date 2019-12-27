#include "ngx_echo_handler_proc.h"

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
	ngx_str_t uri;
	ngx_http_handler_pt handler;
} echo_handler;

typedef ngx_int_t (*echo_tester_pt)();
typedef struct {
	ngx_str_t name;
	echo_tester_pt handler;
} echo_tester;

static ngx_int_t handler_default(ngx_http_request_t *r);

static ngx_int_t handler_ver(ngx_http_request_t *r);
static ngx_int_t handler_type(ngx_http_request_t *r);

static echo_handler uri_handlers[] = {
	{ ngx_string("/echo/ver"), handler_ver },
	{ ngx_string("/echo/type"), handler_type },
};


static ngx_int_t test_queue();
static ngx_int_t test_pool();

static echo_tester testers[] = {
	{ ngx_string("ngx_queue_t"), test_queue },
	{ ngx_string("ngx_pool_t"), test_pool },
};

static ngx_int_t ngx_str_append(ngx_str_t* o, ngx_int_t maxlen, char* str, ngx_int_t len){
	u_char* dst = o->data + o->len;
	ngx_int_t l = o->len + len;
	if( l >= maxlen ){
		return -1; //out of buff max size
	}

	ngx_memcpy(dst, str, len);
	o->len = l;
	return 0;
}

static ngx_int_t ngx_str_appendf(ngx_str_t* o, ngx_int_t maxlen, char* fmt, ...){
	char buf[512];
	va_list args;
	va_start(args, fmt);
	ngx_int_t len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end (args);

	ngx_int_t c = ngx_str_append(o, maxlen, buf, len);
	return c;
}

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

	static const ngx_int_t nhandler = sizeof(uri_handlers)/sizeof(uri_handlers[0]);

	u_char buf[2048]; ngx_memzero(buf, sizeof(buf));

	ngx_str_t res;
	res.data = buf;
	res.len = 0;

	ngx_str_appendf( &res, sizeof(buf), "uri help:\n" );

	echo_handler* eh = NULL;
	for( ngx_int_t i=0; i<nhandler; ++i ){
		eh = uri_handlers + i;

		ngx_str_appendf( &res, sizeof(buf), "\t%s\n", eh->uri.data );
	}

	ngx_int_t rc = send_response(r, &res);
	return rc;
}

static ngx_int_t handler_ver(ngx_http_request_t *r){

	ngx_str_t res = ngx_string("echo 1.0.0\n");
	ngx_int_t rc = send_response(r, &res);
	return rc;
}

static ngx_int_t handler_type(ngx_http_request_t *r){

	static int ntester = sizeof(testers)/sizeof(testers[0]);
	echo_tester* t = NULL;

	u_char buf[4096]; 
	ngx_memzero(buf, sizeof(buf));

	ngx_str_t res;
	res.data = buf;
	res.len = 0;

	ngx_int_t tc = 0;
	for( int i=0; i<ntester; ++i ){
		t = testers + i;
		tc = t->handler();

		if( 0 == tc ){
			ngx_str_appendf(&res, sizeof(buf), "T %2d %-10s\n", i, t->name.data);
		}else{
			ngx_str_appendf(&res, sizeof(buf), "F %2d %-10s\n", i, t->name.data);
		}
	}

	ngx_int_t rc = send_response(r, &res);
	return rc;
}

static ngx_int_t test_queue(){


	return 0;
}

static ngx_int_t test_pool(){

	return 0;
}

ngx_int_t ngx_echo_handler_proc(ngx_http_request_t *r){

	static const ngx_int_t nhandler = sizeof(uri_handlers)/sizeof(uri_handlers[0]);

	ngx_str_t* puri = &(r->uri);

	ngx_int_t rc = 0;
	echo_handler* eh = NULL;
	for( ngx_int_t i=0; i<nhandler; ++i ){
		eh = uri_handlers + i;

		if (!ngx_strncmp(puri->data, eh->uri.data, eh->uri.len)) {
			rc = eh->handler(r);
			return rc;
		}
	}

	rc = handler_default(r);
	return rc;
}