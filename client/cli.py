from threading import Thread
import time
import socket
import sys
import os
import hashlib

g_data = None

def send_data(host, port, data):
#	start = time.time()

	offset = 0
	sent = 0

	soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	soc.connect((host, port))

	while(1):
		sent = soc.send(data[offset:offset+4096])
		offset = offset + sent
	
		if offset == len(data):
			break

#	print 'sent data size ' + str(offset)

#	if soc.sendall(data+'\n') == None:
#		print 'sent data size ' + str(len(data))

#	try:
#		while 1:
#			data = soc.recv(8192)
#			if not data:
#				break
#			return elapse, connect_time
#	except socket.error as msg:
#		print msg

#	soc.close()

	return -1

if __name__ == '__main__':
	ip = '127.0.0.1'
	port = 5555

	if len(sys.argv) < 3:
		print 'cli #_of_thread data_size [ip] [port]'
		sys.exit()

	number_threads = int(sys.argv[1])
	data_size = sys.argv[2]

	if len(sys.argv) >= 4:
		ip = sys.argv[3]
	
	if len(sys.argv) >= 5:
		port = int(sys.argv[4])

	if data_size.find('KB') != -1:
		data_size = data_size[:data_size.find('KB')]
		data_size = int(data_size) * 1024
	elif data_size.find('MB') != -1:
		data_size = data_size[:data_size.find('MB')]
		data_size = int(data_size) * 1048576
	elif data_size.find('GB') != -1:
		data_size = data_size[:data_size.find('GB')]
		data_size = float(data_size) * 1073741824
	else:
		data_size = 0

	g_data = ''
	for i in range(0, data_size/1024):
		g_data = g_data.ljust(len(g_data)+1024, str(i%10))

	m = hashlib.md5()
	m.update(g_data)
	print 'data checksum: ' +  m.hexdigest()

	try:
		thread_list = []
		
		start = time.time()
		for i in range(0, number_threads):
			
			t = Thread(target=send_data, args=(ip, port, g_data,))
			thread_list.append(t)
		#	t.daemon = True
			t.start()

		for thread in thread_list:
			thread.join()

		elapsed = time.time() - start
		
		print 'elapsed : ' +  str(elapsed)
	except KeyboardInterrupt:
		sys.exit(1)
