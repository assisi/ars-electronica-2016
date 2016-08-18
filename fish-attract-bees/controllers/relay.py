#!/usr/bin/env python
# -*- coding: utf-8 -*-


import zmq

if __name__ == '__main__':

    context = zmq.Context(1)
    pub = context.socket(zmq.PUB)
    pub.bind('tcp://*:10103')

    sub = context.socket(zmq.SUB)
    sub.connect('tcp://bbg-001:10201')
    sub.connect('tcp://bbg-001:20202')
    sub.setsockopt(zmq.SUBSCRIBE, 'cats')

    print('Everything is connected!')

    while True:
        [name, msg, sender, data] = sub.recv_multipart()
        print('Received: ' + name + ';' + msg + ';' + sender + ';' + data)

    
