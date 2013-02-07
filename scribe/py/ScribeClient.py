#!/usr/bin/env python
# coding=utf-8
# Filename : ScribeClient.py
# Func : python实现的scribe客户端 用来给nginx c 调用
# author : yinhongzhan@myhexin.com

import os; 
import sys; 
import time;
import thread;
from thrift import Thrift;
from thrift.transport import TTransport; 
from thrift.transport import TSocket;
from thrift.transport.TTransport import TTransportException;  
from thrift.protocol import TBinaryProtocol;
from scribe import scribe;

class Error(Exception): pass;

class Scriber:
	print "Scriber 创建!";

	transport  = None;
	protocol = None;	
	client = None;
    cnt = 0;
	def __init__(self, host="172.16.111.132", port="1463", flush = False):  
		print "初始化...";
		self.config(host, port, flush);
		self.open();

	def __del__(self) : 
		self.close();

	def config(self, host="172.16.111.132", port="1463", flush = False):
		try:
			if Scriber.transport == None or flush :
				Scriber.transport = TSocket.TSocket(host, port);
				Scriber.transport = TTransport.TBufferedTransport(Scriber.transport);
				Scriber.transport = TTransport.TFramedTransport(Scriber.transport)  
				Scriber.protocol  = TBinaryProtocol.TBinaryProtocol(Scriber.transport);
				Scriber.client = scribe.Client(iprot=Scriber.protocol, oprot=Scriber.protocol); 
				print "client创建成功: host = %s , port = %s" % (host, port);
			else :
				print "client已经创建 :",  Scriber.transport.getTrans()._resolveAddr();
		except Thrift.TException, tx:
			print "client创建失败: %s" % (tx.message);

	def open(self) :
		try:
			Scriber.transport.open();
		except Thrift.TException, tx:
			print "client open失败: %s" % (tx.message);

	def close(self) :
		try:
			Scriber.transport.close();
		except Thrift.TException, tx:
			print "client close失败: %s" % (tx.message);

	def slog(self, category, message) :
		print('slog... category = %s , message = %s' % (category, message)); 
		log_entry = scribe.LogEntry(category, message + str(ctn++)); 
		result = Scriber.client.Log(messages=[log_entry]);
		if result == scribe.ResultCode.OK : 
			pass;
		elif result == scribe.ResultCode.TRY_LATER :  
			print('Scribe Error: TRY LATER'); 
		else:  
			print('Scribe Error: Unknown error code (%s)' % result); 

	def mlog(self, category, message) : 
		try:	 
			thread.start_new_thread(self.slog, (category, message));
		except Thrift.TException, tx:
			print "client log 失败: %s" % (tx.message);
#
#scriber1 = Scriber();
#for j in range(0, 100) :
#	msg = "";
#	for i in range(0, 1000) :
#		msg += time.strftime('%H%M%S') + "\n";
#	scriber1.slog("default", msg);

#time.sleep(30);
