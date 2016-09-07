#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
A program for debugging the casu-cats interconnection.
"""

import zmq

import threading
import time

class Relay:

    def __init__(self):
        ''' Create and connect sockets '''
        self.context = zmq.Context(1)

        self.sub_cats = self.context.socket(zmq.SUB)
        # Connect the address to listen to CATS
        self.sub_cats.connect('tcp://10.42.0.201:10203')
        self.sub_cats.setsockopt(zmq.SUBSCRIBE,'casu-001')
        self.sub_cats.setsockopt(zmq.SUBSCRIBE,'casu-002')
        self.sub_cats.setsockopt(zmq.SUBSCRIBE,'FishPosition')
        self.sub_cats.setsockopt(zmq.SUBSCRIBE,'CASUPosition')
        print('Cats subscriber bound!')

        """
        self.pub_internet = self.context.socket(zmq.PUB)
        # Bind the address to publish to CATS
        self.pub_internet.bind('tcp://*:5555')
        print('Internet publisher bound!')
        """

        """
        self.pub_local = self.context.socket(zmq.PUB)
        self.pub_local.bind('tcp://*:10103')
        print('Local publisher bound!')
        """

        self.sub_casu = self.context.socket(zmq.SUB)
        self.sub_casu.connect('tcp://bbg-001:10101')
        self.sub_casu.connect('tcp://bbg-001:10102')
        self.sub_casu.setsockopt(zmq.SUBSCRIBE,'cats')
        print('Casu message subscribers bound!')
    
        self.cats_thread = threading.Thread(target = self.recieve_from_cats)
        self.casu_thread = threading.Thread(target = self.recieve_from_casu)

        self.stop = False

        self.cats_thread.start()
        self.casu_thread.start()

    def recieve_from_cats(self):
        while not self.stop:
            [name, msg, sender, data] = self.sub_cats.recv_multipart()
            print('Received from cats: ' + name + ';' + msg + ';' + sender + ';' + data)
            #self.pub_local.send_multipart([name,msg,sender,data])

    def recieve_from_casu(self):
        while not self.stop:
            [name, msg, sender, data] = self.sub_casu.recv_multipart()
            print('Received from arena: ' + name + ';' + msg + ';' + sender + ';' + data)
            #self.pub_internet.send_multipart([name,msg,sender,data])
        
if __name__ == '__main__':

    relay = Relay()

    cmd = 'a'
    while cmd != 'q':
        cmd = raw_input('To stop the program press q<Enter>')

    relay.stop = True

    
