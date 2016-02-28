import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 7002))


BLOCK = 100000.0
SIZE = 10000000.0

for i in range(int(SIZE/BLOCK)):
    s.send('a' * int(BLOCK))
