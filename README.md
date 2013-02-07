hexin-nginx-module
==================

存放nginx开发的组件
==================

  scribe文件夹
==================
  
  nginx通过调用python 脚本实现 scribe的客户端功能。
  
  外部通过发起http请求将需要记录的信息发送给nginx,nginx通过python客户端(启动时会与scribe服务端建立tcp连接)脚本将数据写入
  到后台进行存储。
  
  使用方式:配置nginx.conf
          
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
  
  percn文件夹
==================
  
  感知负载组件，主要通过配置参数$block_size和POST表单的请求中的参数receivers将一个请求切割成 
  
             count(receicers) / blocksize  +  1 
             
  个 subrequest 发送给上有服务器群。
  
  使用方式:配置nginx.conf
  
            upstream percn_test {
            		server 192.168.27.23:8088;
                #server 192.168.27.23:8081;
            }

            location /percn {	
                error_log  logs/error.log debug;	

                set_form_input $recievers;
                set_form_input $msgId;
                set_form_input $sender;
                set_form_input $createTime;
            		set_form_input $liveTime;
            		set_form_input $title;
            		set_form_input $content;
            		set_form_input $format;
            		set_form_input $msgType;
            		set_form_input $blackList;
            		set_form_input $blackListType;	
            		set_form_input $stationTypes;
            		set_form_input $stationCodes;

            		set $block_size 2;
            		set $common "&msgId=$msgId&sender=$sender&createTime=$createTime&liveTime=$liveTime&title=&title&content=$content&format=$format&msgType=$msgType&blackList=$blackList&blackListType=$blackListType&stationTypes=$stationTypes&stationCodes=$stationCodes";
            
            		percn_pass percn_test;
            } 

  
  
  
