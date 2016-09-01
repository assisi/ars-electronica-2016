#!/usr/bin/env python
# -*- coding: utf-8 -*-


import zmq

import threading
import time

class Relay:

    def __init__(self):
        ''' Create and connect sockets '''
        self.context = zmq.Context(1)

        self.sub_internet = self.context.socket(zmq.SUB)
        # Bind the address to listen to CATS
        self.sub_internet.bind('tcp://*:5556')
        self.sub_internet.setsockopt(zmq.SUBSCRIBE,'casu-001')
        self.sub_internet.setsockopt(zmq.SUBSCRIBE,'casu-002')
        print('Internet subscriber bound!')

        self.pub_internet = self.context.socket(zmq.PUB)
        # Bind the address to publish to CATS
        self.pub_internet.bind('tcp://*:5555')
        print('Internet publisher bound!')

        self.pub_local = self.context.socket(zmq.PUB)
        self.pub_local.bind('tcp://*:10103')
        print('Local publisher bound!')

        self.sub_local = self.context.socket(zmq.SUB)
        self.sub_local.connect('tcp://bbg-001:10201')
        self.sub_local.connect('tcp://bbg-001:10202')
        self.sub_local.setsockopt(zmq.SUBSCRIBE,'cats')
        print('Local subscribers bound!')
    
        self.incoming_thread = threading.Thread(target = self.recieve_from_internet)
        self.outgoing_thread = threading.Thread(target = self.recieve_from_local)

        self.stop = False

        self.incoming_thread.start()
        self.outgoing_thread.start()

    def recieve_from_internet(self):
        while not self.stop:
            [name, msg, sender, data] = self.sub_internet.recv_multipart()
            print('Received from cats: ' + name + ';' + msg + ';' + sender + ';' + data)
            self.pub_local.send_multipart([name,'Message','cats',data])

    def recieve_from_local(self):
        while not self.stop:
            [name, msg, sender, data] = self.sub_local.recv_multipart()
            print('Received from arena: ' + name + ';' + msg + ';' + sender + ';' + data)
            self.pub_internet.send_multipart(['cats','Message',name,data])
        
if __name__ == '__main__':

    relay = Relay()

    cmd = 'a'
    while cmd != 'q':
        cmd = raw_input('To stop the program press q<Enter>')

    relay.stop = True

    
