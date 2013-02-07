
/*
 * add by yinhongzhan@myhexin.com
 * 
 */
#ifndef HEXIN_HTTP_PERCN_TEMP_PATH
#define HEXIN_HTTP_PERCN_TEMP_PATH  "percn_temp"
#endif

#define DDEBUG 0
#include "../form-input-nginx-module/src/ddebug.h"

#include <ndk.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define hexin_http_percn_method_name(m) { sizeof(m) - 1, (u_char *) m " " }
ngx_str_t  hexin_http_percn_post_method = hexin_http_percn_method_name("POST");

typedef struct {
	ngx_int_t					block_size_index;
	ngx_int_t					receivers_index;
	ngx_int_t					common_index;
	ngx_str_t					method;

	 ngx_array_t     *handler_cmds;
    ngx_array_t     *before_body_cmds;
    ngx_array_t     *after_body_cmds;

    unsigned         seen_leading_output;

    ngx_int_t        status;
} hexin_http_percn_loc_conf_t;


typedef struct {
	size_t					   receivers_complete_val_len;
	size_t					   receivers_part_val_len;
	u_char					   *receivers_part_val;
	ngx_http_variable_value_t  *common;    
} hexin_http_percn_ctx_t;

typedef struct hexin_http_percn_subrequest_s {
    ngx_uint_t                  method;
    ngx_str_t                   *method_name;
    ngx_str_t                   *location;
    ngx_str_t                   *query_string;
    ssize_t                     content_length_n;
    ngx_http_request_body_t     *request_body;
} hexin_http_percn_subrequest_t;

static char *hexin_http_percn_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *hexin_http_percn_create_loc_conf(ngx_conf_t *cf);
static char *hexin_http_percn_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_command_t  hexin_http_percn_commands[] = {

    { ngx_string("percn_pass"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      hexin_http_percn_pass,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  hexin_http_percn_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                 /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    hexin_http_percn_create_loc_conf,		/* create location configuration */
    hexin_http_percn_merge_loc_conf			/* merge location configuration */
};


ngx_module_t  hexin_http_percn_module = {
    NGX_MODULE_V1,
    &hexin_http_percn_module_ctx,        /* module context */
    hexin_http_percn_commands,           /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_str_t  hexin_http_percn_form_receivers = ngx_string("recievers");
static ngx_str_t  hexin_http_percn_form_common = ngx_string("common");
static ngx_str_t  hexin_http_percn_block_size = ngx_string("block_size");
static ngx_str_t  hexin_http_percn_content_length_header_key = ngx_string("Content-Length");
static ngx_str_t  subpath = ngx_string("/");

static ngx_str_t  post_method_name = ngx_string("POST");

static ngx_int_t hexin_http_percn_parse_method_name(ngx_str_t **method_name_ptr)
{
    *method_name_ptr = &hexin_http_percn_post_method;
    return NGX_HTTP_POST;    
}

static ngx_int_t hexin_http_percn_set_content_length_header(ngx_http_request_t *r, off_t len)
{
    ngx_table_elt_t                 *h, *header;
    u_char                          *p;
    ngx_list_part_t                 *part;
    ngx_http_request_t              *pr;
    ngx_uint_t                       i;

    r->headers_in.content_length_n = len;

    if (ngx_list_init(&r->headers_in.headers, r->pool, 20, sizeof(ngx_table_elt_t)) != NGX_OK) {
        return NGX_ERROR;
    }

    h = ngx_list_push(&r->headers_in.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->key = hexin_http_percn_content_length_header_key;
    h->lowcase_key = ngx_pnalloc(r->pool, h->key.len);
    if (h->lowcase_key == NULL) {
        return NGX_ERROR;
    }

    ngx_strlow(h->lowcase_key, h->key.data, h->key.len);

    r->headers_in.content_length = h;

    p = ngx_palloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    h->value.data = p;

    h->value.len = ngx_sprintf(h->value.data, "%O", len) - h->value.data;

    h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(
            ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(
            ngx_hash('C', 'o'), 'n'), 't'), 'e'), 'n'), 't'), '-'), 'L'), 'e'),'n'), 'g'), 't'), 'h');

    pr = r->parent;

    if (pr == NULL) {
        return NGX_OK;
    }


    part = &pr->headers_in.headers.part;
    header = part->elts;

	for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].key.len == sizeof("Content-Length") - 1 && ngx_strncasecmp(header[i].key.data, (u_char *) "Content-Length", sizeof("Content-Length") - 1) == 0)
        {
           continue;
        }

        h = ngx_list_push(&r->headers_in.headers);
        if (h == NULL) {
            return NGX_ERROR;
        }

        *h = header[i];
    }

    return NGX_OK;
}


static ngx_int_t hexin_http_percn_create_subrequest(ngx_http_request_t *r, hexin_http_percn_subrequest_t **parsed_sr_ptr)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "进入 hexin_http_percn_create_subrequest");

	size_t							body_len;
    ngx_buf_t						*b;
    ngx_chain_t						*cl;
    hexin_http_percn_ctx_t			*ctx;
    hexin_http_percn_subrequest_t	*parsed_sr;
	ngx_http_request_body_t			*rb;

	*parsed_sr_ptr = ngx_pcalloc(r->pool, sizeof(hexin_http_percn_subrequest_t));
    if (*parsed_sr_ptr == NULL) {
        return NGX_ERROR;
    }
    parsed_sr = *parsed_sr_ptr;
	parsed_sr->location = &subpath; 
	
	ngx_str_t	*method_name;	
	method_name = &post_method_name;
    parsed_sr->method = hexin_http_percn_parse_method_name(&method_name);
	parsed_sr->method_name = method_name;

	ctx = ngx_http_get_module_ctx(r, hexin_http_percn_module);

	body_len = ngx_strlen("receivers=") + ctx->receivers_part_val_len + ctx->common->len;

	rb = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));

    if (rb == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    parsed_sr->content_length_n = body_len;
   
    b = ngx_create_temp_buf(r->pool, body_len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->temporary = 1;
    /* b->memory = 1; */

    cl = ngx_alloc_chain_link(r->pool);
    if (cl == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    cl->buf = b;
	cl->next = NULL;
	rb->buf = b;
	rb->bufs = cl;
	*b->last++ = 'r'; *b->last++ = 'e'; *b->last++ = 'c'; *b->last++ = 'e';
	*b->last++ = 'i'; *b->last++ = 'v'; *b->last++ = 'e'; *b->last++ = 'r';
	*b->last++ = 's'; *b->last++ = '='; 
    b->last = ngx_copy(b->last, ctx->receivers_part_val,  ctx->receivers_part_val_len);	
	b->last = ngx_copy(b->last, ctx->common->data,  ctx->common->len);

	parsed_sr->request_body = rb;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http percn body:\n\"%*s\"", (size_t) (b->last - b->pos), b->pos);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "hexin_http_percn_create_subrequest 结束");

    return NGX_OK;
}

static ngx_int_t hexin_http_percn_adjust_subrequest(ngx_http_request_t *sr, hexin_http_percn_subrequest_t *parsed_sr)
{
    ngx_http_core_main_conf_t  *cmcf;
    ngx_http_request_t         *r;
    ngx_http_request_body_t    *body;
	ngx_int_t                   rc; 
 
	sr->method = parsed_sr->method;
    sr->method_name = *(parsed_sr->method_name);
    r = sr->parent;

    sr->header_in = r->header_in;

    if (r->headers_in.headers.last == &r->headers_in.headers.part) {
        sr->headers_in.headers.last = &sr->headers_in.headers.part;
    }

    cmcf = ngx_http_get_module_main_conf(sr, ngx_http_core_module);

    body = parsed_sr->request_body;
    if (body) {
        sr->request_body = body;
	
        rc = hexin_http_percn_set_content_length_header(sr, body->buf ? ngx_buf_size(body->buf) : 0);

        if (rc != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

    }

    return NGX_OK;
}

static ngx_int_t hexin_http_percn_post_handler(ngx_http_request_t *r)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "进入 hexin_http_percn_post_handler 方法..");

	hexin_http_percn_loc_conf_t		*plcf;
	hexin_http_percn_ctx_t			*ctx;
	ngx_http_variable_value_t		*receivers, *common, *block_size;
	u_char							*buf, *unescapebuf;
	ngx_int_t						rc;

	plcf = ngx_http_get_module_loc_conf(r, hexin_http_percn_module);	
	ctx = ngx_http_get_module_ctx(r, hexin_http_percn_module);
	//获得分割所依据的字段值，并获得所以子请求通用的Form参数
	receivers = ngx_http_get_indexed_variable(r, plcf->receivers_index);	
	common =  ngx_http_get_indexed_variable(r, plcf->common_index);	
              
	ctx->common = common;
	ctx->receivers_complete_val_len = receivers->len;

	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "接收者为: %v, ----- 通用体: %v", receivers, common);		
		
    buf = ngx_pnalloc(r->pool, receivers->len);
	if(buf == NULL) {
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "分配 buf 内存失败!");
		return NGX_ERROR;
	}

    ngx_memcpy(buf, receivers->data, receivers->len);
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "URL 解码前: %s", buf);
	//进行URI 解码
	unescapebuf = ngx_pnalloc(r->pool, receivers->len);
	ngx_unescape_uri(&unescapebuf, &buf,  receivers->len, NGX_ESCAPE_ARGS);
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "URL 解码后: %s", buf);
	//获取分割的尺寸大小	
	block_size = ngx_http_get_indexed_variable(r, plcf->block_size_index);	
	ngx_int_t block_size_int = ngx_atoi(block_size->data, block_size->len);

	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "推送块尺寸为: %d", block_size_int);
	//以','为分割符进行切割 并循环创建子请求
	char *p, *brkt;
	ngx_int_t count = 1;

	size_t len = 0;
	size_t fla_len = 0;
	u_char* next = receivers->data;
	for (p = strtok_r( (char *)buf, ",", &brkt); p;  p = strtok_r(NULL, ",", &brkt)) {
		len += ngx_strlen(p);
		fla_len++;
		if((count++) % (block_size_int) == 0) {	
			//获得块长度
			u_char* part ;
			size_t mem_len =  len + 3 * fla_len;
			//分配块内存 并拷贝内存赋值
			part = ngx_pnalloc(r->pool, mem_len - 3);
			ngx_memcpy(part, next, mem_len -3);	
			next = next + mem_len;
			ctx->receivers_part_val_len = mem_len - 3;
			ctx->receivers_part_val = part;			

			//=================
			hexin_http_percn_subrequest_t      *parsed_sr;
			ngx_http_request_t				   *sr; /* subrequest object */
			
			rc = hexin_http_percn_create_subrequest(r, &parsed_sr);
			if (rc != NGX_OK) {
				return rc;
			}
			
			ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "创建子查询.");
			rc = ngx_http_subrequest(r, parsed_sr->location, NULL, &sr, NULL, 0);
			if (rc != NGX_OK) {
				return NGX_ERROR;
			}	
			
			ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "创建子查询结束， 开始调整参数.");	
			rc = hexin_http_percn_adjust_subrequest(sr, parsed_sr);
			if (rc != NGX_OK) {
				return rc;
			}
			//=================
			len = 0;
			fla_len = 0;
		}
    }
	//剩余请求
	if(len != 0) {
		u_char* part ;
		size_t mem_len =  len + 3 * fla_len;
		part = ngx_pnalloc(r->pool, mem_len - 3);
		ngx_memcpy(part, next, mem_len -3);
		ctx->receivers_part_val_len = mem_len - 3;
		ctx->receivers_part_val = part;

		//=================
		hexin_http_percn_subrequest_t      *parsed_sr;
		ngx_http_request_t				   *sr; /* subrequest object */
			
		rc = hexin_http_percn_create_subrequest(r, &parsed_sr);
		if (rc != NGX_OK) {
			return rc;
		}

		rc = ngx_http_subrequest(r, parsed_sr->location, NULL, &sr, NULL, 0);
		if (rc != NGX_OK) {
			return NGX_ERROR;
		}	
						
		rc = hexin_http_percn_adjust_subrequest(sr, parsed_sr);
		if (rc != NGX_OK) {
			return rc;
		}
		//=================

		len = 0;
	}

	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, " hexin_http_percn_post_handler 方法 结束..");

	return NGX_OK;
}


static ngx_int_t hexin_http_percn_handler(ngx_http_request_t *r)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "进入 hexin_http_percn_handler 方法..");

	if(!(r->method & (NGX_HTTP_POST))) {
		 return NGX_HTTP_NOT_ALLOWED;	
	}

	if (ngx_http_set_content_type(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    hexin_http_percn_loc_conf_t  *plcf;
	plcf = ngx_http_get_module_loc_conf(r, hexin_http_percn_module);

	r->state = 0;	

	hexin_http_percn_ctx_t       *ctx;
    ctx = ngx_palloc(r->pool, sizeof(hexin_http_percn_ctx_t));
  
  	if (ctx == NULL) {
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "分配 hexin_http_percn_ctx_t 内存失败!");
        return  NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, hexin_http_percn_module);
	ngx_int_t rc;
	rc = hexin_http_percn_post_handler(r);

	if (rc != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }	

	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "hexin_http_percn_handler 结束..");
	//ngx_http_finalize_request(r, NGX_OK);
	return NGX_DECLINED;
}

static char *hexin_http_percn_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    hexin_http_percn_loc_conf_t *plcf = conf;

    ngx_str_t                 *value;
    ngx_url_t                  u;
    ngx_http_core_loc_conf_t  *clcf;

    value = cf->args->elts;

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.no_resolve = 1;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    clcf->handler = hexin_http_percn_handler;

    if (clcf->name.data[clcf->name.len - 1] == '/') {
        clcf->auto_redirect = 1;
    }

    plcf->receivers_index = ngx_http_get_variable_index(cf, &hexin_http_percn_form_receivers);
	plcf->common_index = ngx_http_get_variable_index(cf, &hexin_http_percn_form_common);	
	plcf->block_size_index = ngx_http_get_variable_index(cf, &hexin_http_percn_block_size);


    if (plcf->receivers_index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static void *
hexin_http_percn_create_loc_conf(ngx_conf_t *cf)
{
    hexin_http_percn_loc_conf_t        *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(hexin_http_percn_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    conf->status = NGX_CONF_UNSET;

    return conf;
}


static char *
hexin_http_percn_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    hexin_http_percn_loc_conf_t    *prev = parent;
    hexin_http_percn_loc_conf_t    *conf = child;

    if (conf->handler_cmds == NULL) {
        conf->handler_cmds = prev->handler_cmds;
        conf->seen_leading_output = prev->seen_leading_output;
    }

    if (conf->before_body_cmds == NULL) {
        conf->before_body_cmds = prev->before_body_cmds;
    }

    if (conf->after_body_cmds == NULL) {
        conf->after_body_cmds = prev->after_body_cmds;
    }

    ngx_conf_merge_value(conf->status, prev->status, 200);

    return NGX_CONF_OK;
}
