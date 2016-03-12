#! /usr/local/bin/python3

import socket
import threading
import message_pb2

PORT = 7002

class Client(object):
    def __init__(self, name):
        self.s = self.__get_socket()
        self.__register(name)
        self.buf = []

        # TODO shutdown thread 
        t = threading.Thread(target=self.__recv)
        t.start()

    def __get_socket(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', PORT))
        return s

    def __encode(self, msg):
        size = len(msg)
        array = [0 for i in range(4)]
        array[0] = (size>>24) & 0xff
        array[1] = (size>>16) & 0xff
        array[2] = (size>>8) & 0xff
        array[3] = (size & 0xff)
        array = [chr(x) for x in array]
        return (''.join(array) + msg)

    def __decode(self, buf):
        assert(len(buf) == 4)
        size = 0
        buf = [int(x.encode('hex'), 16) for x in buf]
        size += (buf[0] << 24)
        size += (buf[1] << 16)
        size += (buf[2] << 8)
        size += (buf[3])
        return size

    def __register(self, name):
        req = message_pb2.Request()
        req.type = req.Authentication
        req.authentication.name = name
        self.s.send(self.__encode(req.SerializeToString()))

    def send(self, to, content):
        req = message_pb2.Request()
        req.type = req.Message
        req.message.content = content
        req.message.recipient = to
        self.s.send(self.__encode(req.SerializeToString()))

    def __parse_proto(self, msg):
        res = message_pb2.Response()
        res.ParseFromString(''.join(msg))
        self.__handle_response(res)

    def __handle_response(self, res):
        if res.type == res.Error:
            print 'Error', res.error
        if res.type == res.Message:
            print 'Message from {}: {}'.format(res.message.sender, res.message.content)

    def __recv(self):
        while True:
            self.buf.extend(self.s.recv(10000))
            while len(self.buf) >= 4:
                size = self.__decode(self.buf[:4])
                if len(self.buf) - 4 >= size:
                    self.buf = self.buf[4:]
                    self.__parse_proto(self.buf[:size])
                    self.buf = self.buf[size:]

