#! /usr/local/bin/python3

import socket
import message_pb2

PORT = 7002

class Client(object):
    def __init__(self, name):
        self.s = self.get_socket()
        self.register(name)

    def get_socket(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', PORT))
        return s

    def encode(self, msg):
        size = len(msg)
        array = [0 for i in range(4)]
        array[0] = (size>>24) & 0xff
        array[1] = (size>>16) & 0xff
        array[2] = (size>>8) & 0xff
        array[3] = (size & 0xff)
        array = [chr(x) for x in array]
        return (''.join(array) + msg)

    def register(self, name):
        req = message_pb2.Request()
        req.type = req.Authentication
        req.authentication.name = name
        self.s.send(self.encode(req.SerializeToString()))

    def send(self, content, to):
        req = message_pb2.Request()
        req.type = req.Message
        req.message.content = content
        req.message.recipient = to
        self.s.send(self.encode(req.SerializeToString()))


