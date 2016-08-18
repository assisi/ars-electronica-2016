#!/usr/bin/env python
# -*- coding: utf-8 -*-

# A simple controller which listens to incoming messages
# and heats up every time that all incoming messages
# match the provided argument

import sys
import time

from assisipy import casu

if __name__ == '__main__':

    if len(argv) < 3:
        sys.exit('Please invoke the program as follows: ' +
                 'listen_and_heat.py rtcfile.rtc <left/right>')

    c = casu.Casu(argv[1])

    myside = argv[2]
    print('My side is {0}'.format(myside))

    while True:
        msg = c.read_message()
        if msg:
            print(msg)
            data = msg['data'].split(',')
        time.sleep(0.5)
