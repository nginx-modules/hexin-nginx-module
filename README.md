hexin-nginx-module
==================

存放nginx开发的组件

  scribe
  =========
  nginx通过调用python 脚本实现 scribe的客户端功能。
  外部通过发起http请求将需要记录的信息发送给nginx,nginx通过python客户端(启动时会与scribe服务端建立tcp连接)脚本将数据写入
  到后台进行存储。
        使用方式:
          nginx.conf
          
              /scribe {
                  error_log  logs/error.log debug;	
	
		              set $message "$args&custom_ip=$remote_addr&request_time=";

		              scribe_host 192.168.1.122;
		              scribe_port 1463;
		              scribe_category default;
		              scribe_python_workspace "sys.path.append('/usr/local/nginx/sbin/py/')";
		              scribe_python_filename ScribeClient;
		              scribe_python_classname Scriber;
		              scriber;
              }
