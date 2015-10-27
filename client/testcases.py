from threading import Thread
import time
import socket
import sys
import os
import random

def send_data(ip, port, data):

    offset = 0
    sent = 0

    soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    soc.connect((ip, port))

    while(1):
        sent = soc.send(data[offset:offset+4096])
        offset = offset + sent
	
        if offset == len(data):
            break

    return -1


def create_threads(num_threads, ip, port, size, vary):
    try:
        thread_list = []
		
        if vary:
            size = random.randrange(1, 10240)
   
        start = time.time()
        for i in range(0, num_threads):
   
            data = ''
            for i in range(0, size):
                data = data.ljust(len(data)+1024, str(i%10))  
  
            t = Thread(target=send_data, args=(ip, port, data,))
            thread_list.append(t)
            t.start()

        for thread in thread_list:
            thread.join()

        elapsed = time.time() - start
		
        print 'Seconds elapsed: ' +  str(elapsed)
    except KeyboardInterrupt:
        sys.exit(1)

def run_test_cases(test_case):
    size = 0

    if test_case is 5:
        print '---------- Test Case 5: Random Sizes ----------'
        vary = True
    else:
        vary = False

    if test_case is 1:
        size = 1
        print '---------- Test Case 1: 1 KB ----------'
    elif test_case is 2:
        size = 100
        print '---------- Test Case 2: 100 KB ----------'
    elif test_case is 3:
        size = 1024
        print '---------- Test Case 3: 1 MB ----------'
    elif test_case is 4:
        size = 10240
        print '---------- Test Case 4: 10 MB ----------'
    
    print '1 Thread'
    
    create_threads(1, ip, port, size, vary)

    print '10 Threads'

    create_threads(10, ip, port, size, vary)

    print '100 Threads'

    create_threads(100, ip, port, size, vary)

    print '1000 Threads'

    create_threads(1000, ip, port, size, vary)

    print '10000 Threads'

    create_threads(10000, ip, port, size, vary)

if __name__ == '__main__':

    if len(sys.argv) < 4:
        print 'test_cases.py [ip] [port] [test case]'
        print 'Test cases: '
        print '1: 1 KB data size'
        print '2: 100 KB data size'
        print '3: 1 MB data size'
        print '4: 10 MB data size'
        print '5: Variable data size'
        sys.exit()

    if len(sys.argv) == 4:
        ip = sys.argv[1]
        port = int(sys.argv[2])
        test_case = int(sys.argv[3])  

    print 'Operating system: ' + sys.platform
    
    run_test_cases(test_case)
