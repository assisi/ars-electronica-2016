#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import time

from assisipy import casu

if __name__ == '__main__':

    '''
    if len(sys.argv) < 2:
        sys.exit('Please invoke the program as follows: ' +
                 'fake_cats.py rtcfile.rtc')

    c = casu.Casu(sys.argv[1])
    
    message_list = ['fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:right',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:left',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:left',
                    'fish-01:right,fish-02:left,fishCasu-01:left,fishCasu-02:left',
                    'fish-01:left,fish-02:left,fishCasu-01:left,fishCasu-02:left']

    td = 1 # Sample time for publishing messages
    
    # Publish all messages in the list
    for msg in message_list:
        #print('Sending message: ' + msg)
        c.send_message('casu-001',msg)
        c.send_message('casu-002',msg)
        time.sleep(td)

    # Once all messages have been published,
    # keep publishing the last message
    while True:
        #print('Sending message: ' + msg)
        c.send_message('casu-001',message_list[-1])
        c.send_message('casu-002',message_list[-1])
        time.sleep(td)
    '''
    pass
