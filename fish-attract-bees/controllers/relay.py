#!/usr/bin/env python
# -*- coding: utf-8 -*-


import zmq

if __name__ == '__main__':

    context = zmq.Context(1)
    pub = context.socket(zmq.PUB)
    pub.bind('tcp://*:10103')
    print('Publisher bound!')

    sub = context.socket(zmq.SUB)
    # Connect the address to listen to CATS
    sub.bind('tcp://143.50.158.98:5556')
    sub.setsockopt(zmq.SUBSCRIBE,'casu-001')
    sub.setsockopt(zmq.SUBSCRIBE,'casu-002')
    """
    sub.connect('tcp://*:10201')
    print('Connected left!')
    sub.connect('tcp://*:20202')
    print('Connected right!')
    sub.setsockopt(zmq.SUBSCRIBE, 'cats')
    """
    print('Connected all subscribers!')

    while True:
        [name, msg, sender, data] = sub.recv_multipart()
        print('Sending: ' + name + ';' + msg + ';' + sender + ';' + data)
        pub.send_multipart([name,'Message','cats',data])
    
