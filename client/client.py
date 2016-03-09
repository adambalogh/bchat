#! /usr/local/bin/python3

import socket
import message_pb2

PORT = 7002

def encode(msg):
    size = len(msg)
    array = [0 for i in range(4)]
    array[0] = (size>>24) & 0xff
    array[1] = (size>>16) & 0xff
    array[2] = (size>>8) & 0xff
    array[3] = (size & 0xff)
    array = [chr(x) for x in array]
    return (''.join(array) + msg)

def register(s, name):
    auth = message_pb2.Authentication()
    auth.name = name
    s.send(encode(auth.SerializeToString()))

def send(s, content, to):
    msg = message_pb2.Message()
    msg.content = content
    msg.recipient = to
    s.send(encode(msg.SerializeToString()))

def get_socket():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', PORT))
    return s


s = get_socket()

register(s, 'Adam')
send(s, 'hello', 'Adam')
