
/*
 * author by yinhongzhan@myhexin.com
 * 
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "python/Python.h"


typedef struct {
	PyObject					*pModule;
   	PyObject					*pDict;
	PyObject					*pScriberClient;
	PyObject					*pScriberClientClass;

	ngx_http_complex_value_t	success_response;
	ngx_int_t					isinit;
} hexin_http_scribe_python_t;

typedef struct {
	ngx_str_t					scribe_host;
	ngx_str_t					scribe_port;
	ngx_str_t					scribe_category;
	ngx_str_t                   scribe_python_workspace;
	ngx_str_t					scribe_python_filename;
	ngx_str_t					scribe_python_classname;
    ngx_int_t					status;
	ngx_int_t					message_index;
	
} hexin_http_scribe_loc_conf_t;

static char *hexin_http_scribe_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *hexin_http_scribe_create_loc_conf(ngx_conf_t *cf);
static char *hexin_http_scribe_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_command_t  hexin_http_scribe_commands[] = {

	{ ngx_string("scribe_host"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(hexin_http_scribe_loc_conf_t, scribe_host),
      NULL },

	{ ngx_string("scribe_port"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(hexin_http_scribe_loc_conf_t, scribe_port),
      NULL },

	{ ngx_string("scribe_category"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(hexin_http_scribe_loc_conf_t, scribe_category),
      NULL },
	
	{ ngx_string("scribe_python_workspace"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(hexin_http_scribe_loc_conf_t, scribe_python_workspace),
      NULL },

	{ ngx_string("scribe_python_filename"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(hexin_http_scribe_loc_conf_t, scribe_python_filename),
      NULL },
	
	{ ngx_string("scribe_python_classname"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(hexin_http_scribe_loc_conf_t, scribe_python_classname),
      NULL },

    { ngx_string("scriber"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_NOARGS,
      hexin_http_scribe_init,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};

static ngx_http_module_t  hexin_http_scribe_module_ctx = {
    NULL,										/* preconfiguration */
    NULL,										/* postconfiguration */

    NULL,										/* create main configuration */
    NULL,										/* init main configuration */

    NULL,										/* create server configuration */
    NULL,										/* merge server configuration */

    hexin_http_scribe_create_loc_conf,		    /* create location configuration */
    hexin_http_scribe_merge_loc_conf			/* merge location configuration */
};

ngx_module_t  hexin_http_scribe_module = {
    NGX_MODULE_V1,
    &hexin_http_scribe_module_ctx,         /* module context */
    hexin_http_scribe_commands,            /* module directives */
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
 
static hexin_http_scribe_python_t *hexin_http_scribe_python;
static ngx_str_t  message_flag = ngx_string("message");
static ngx_str_t  ngx_http_gif_type = ngx_string("text/html");
static ngx_str_t success_submit = ngx_string("记录提交成功");
static ngx_str_t UTF_8 = ngx_string("utf-8");
static ngx_int_t hexin_http_scribe_handler(ngx_http_request_t *r)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "进入 hexin_http_scribe_handler 方法..");
	
	ngx_int_t rc;
	
	if(!(r->method & (NGX_HTTP_GET))) {
		 return NGX_HTTP_NOT_ALLOWED;	
	}

	rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "discard body..");

    if (ngx_http_set_content_type(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }   

	hexin_http_scribe_loc_conf_t  *plcf;
	plcf = ngx_http_get_module_loc_conf(r, hexin_http_scribe_module);

	r->state = 0;	

	ngx_http_variable_value_t		*messages;
	messages = ngx_http_get_indexed_variable(r, plcf->message_index);	

	PyObject_CallMethod(hexin_http_scribe_python->pScriberClient, "slog", "(ss)", plcf->scribe_category.data, messages->data);

	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "hexin_http_scribe_handler 结束..");

	r->headers_out.status = NGX_HTTP_OK;
	r->keepalive = 0;
	r->headers_out.charset = UTF_8;

	return ngx_http_send_response(r, NGX_HTTP_OK, &ngx_http_gif_type, &hexin_http_scribe_python->success_response);
}

static char *hexin_http_scribe_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	hexin_http_scribe_loc_conf_t *loc_conf = conf;
	hexin_http_scribe_python = ngx_palloc(cf->pool, sizeof(hexin_http_scribe_python_t));
		
	Py_Initialize();
	 // 检查初始化是否成功
	if (!Py_IsInitialized() ) {
		return NGX_CONF_ERROR;
	}
	
	PyRun_SimpleString("import sys");   
	PyRun_SimpleString((char*)loc_conf->scribe_python_workspace.data); 
	hexin_http_scribe_python->pModule = PyImport_ImportModule((char*)loc_conf->scribe_python_filename.data); 
	if (!hexin_http_scribe_python->pModule) {  
		printf("can't find ScribeClient.py");  
		getchar();
		return NGX_CONF_ERROR;  
	} 
	hexin_http_scribe_python->pDict = PyModule_GetDict(hexin_http_scribe_python->pModule);  
	if (!hexin_http_scribe_python->pDict) {
		return NGX_CONF_ERROR;  
	}  
	hexin_http_scribe_python->pScriberClientClass = PyDict_GetItemString(hexin_http_scribe_python->pDict, (char*)loc_conf->scribe_python_classname.data);
	if (!hexin_http_scribe_python->pScriberClientClass || !PyClass_Check(hexin_http_scribe_python->pScriberClientClass)) {
		return NGX_CONF_ERROR;  
	}

	PyObject *pArgs = PyTuple_New(3);
	PyTuple_SetItem(pArgs, 0, Py_BuildValue("s",loc_conf->scribe_host.data));
	PyTuple_SetItem(pArgs, 1, Py_BuildValue("s",loc_conf->scribe_port.data));
	PyTuple_SetItem(pArgs, 2, Py_BuildValue("b","False"));
	hexin_http_scribe_python->pScriberClient = PyInstance_New(hexin_http_scribe_python->pScriberClientClass, pArgs, NULL);
	 
	if (!hexin_http_scribe_python->pScriberClient) { 
		return NGX_CONF_ERROR;  
	}
	hexin_http_scribe_python->isinit = 1;
   
	ngx_http_complex_value_t  cv; 
    	ngx_memzero(&cv, sizeof(ngx_http_complex_value_t));
	cv.value.len = success_submit.len;
	cv.value.data = success_submit.data;
	hexin_http_scribe_python->success_response = cv;
	loc_conf->message_index = ngx_http_get_variable_index(cf, &message_flag);

	ngx_http_core_loc_conf_t  *clcf;

    	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    	clcf->handler = hexin_http_scribe_handler;

    	return NGX_CONF_OK;
}

static void *hexin_http_scribe_create_loc_conf(ngx_conf_t *cf)
{	
    hexin_http_scribe_loc_conf_t        *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(hexin_http_scribe_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    conf->status = NGX_CONF_UNSET;

    return conf;
}


static char *hexin_http_scribe_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    hexin_http_scribe_loc_conf_t    *prev = parent;
    hexin_http_scribe_loc_conf_t    *conf = child;

    ngx_conf_merge_value(conf->status, prev->status, 200);
    
	return NGX_CONF_OK;
}

