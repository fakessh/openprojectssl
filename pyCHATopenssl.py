#!/usr/bin/env python
# -*- coding: utf-8 -*-

# pyCHATssl
# author: fakessh @

import socket
import ssl
import threading
import sys

mode = raw_input('Server(s) or Client(c) mode: ')
while (mode=='c' or mode=='s')==False:
	print 'ERROR: Response must be "s" or "c"'
	mode = raw_input('Server(s) or Client(c) mode: ')
if mode=='c':
##	host = raw_input('Host address: ')
##	while len(host.split('.'))!=4 or host.split('.')[0]>'255' or host.split('.')[1]>'255' or host.split('.')[2]>'255' or host.split('.')[3]>'255':
##		print 'ERROR: IP address not formatted correctly'
		host = raw_input('Host address: ')	
port = int(raw_input('Port: '))
while port<0 or port>65535:
	print 'ERROR: Port number out of range'
	port = int(raw_input('Port: '))
conn={}
s=1
r=0
if mode=='c':
	#Start Client
	conn[s] = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	ssl_sock_1 = ssl.wrap_socket(conn[s],server_side=True, certfile="/home/fakessh/ks37777.kimsufi.cert",keyfile="/home/fakessh/ks37777.kimsufi.com.key", ssl_version=ssl.PROTOCOL_SSLv3)

	ssl_sock_1.connect((host,port))
	ssl_sock_1.send('::transmit')
	conn[r] = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	ssl_sock_2 = ssl.wrap_socket(conn[r],server_side=True, certfile="/home/fakessh/ks37777.kimsufi.cert",keyfile="/home/fakessh/ks37777.kimsufi.com.key", ssl_version=ssl.PROTOCOL_SSLv3)

	ssl_sock_2.connect((host,port))
else:
	#Start Server
	socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	socket.bind(('',port))
	socket.listen(5)
	c1,a1 = socket.accept()
	ssl_1 = ssl.wrap_socket(c1, server_side=True, certfile="/home/fakessh/ks37777.kimsufi.com.cert",keyfile="/home/fakessh/ks37777.kimsufi.com.key", ssl_version=ssl.PROTOCOL_SSLv3)
	c2,a2 = socket.accept()
	ssl_2 = ssl.wrap_socket(c2, server_side=True, certfile="/home/fakessh/ks37777.kimsufi.com.cert",keyfile="/home/fakessh/ks37777.kimsufi.com.key", ssl_version=ssl.PROTOCOL_SSLv3)
	host = a1[0]
	if ssl_1.recv(10)=='::transmit':
		ssl_sock_1 = ssl_2
		ssl_sock_2 = ssl_1
	else:
		ssl_sock_1 = ssl_2
		ssl_sock_2 = ssl_1
	del ssl_1, a1, ssl_2, a2, socket
class receive(threading.Thread):
	def run(self):
		while 1:
			data = ssl_sock_2.recv(2048)
			if data:
				print '\033[92m'+data+'\033[0m'
receive().start()
try:
	while 1:
		ssl_sock_1.send(raw_input())
except:
	print 'Disconnecting'
ssl_sock_1.close()
ssl_sock_2.close()
