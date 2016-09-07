#!/usr/bin/env python
# -*- coding: utf-8 -*-

# A simple controller which listens to incoming messages
# and heats up every time that all incoming messages
# match the provided argument

import sys
import time

from assisipy import casu

if __name__ == '__main__':

    temp_min = 29
    temp_max = 36
    temp_set = temp_min

    if len(sys.argv) < 3:
        sys.exit('Please invoke the program as follows: ' +
                 'listen_and_heat.py rtcfile.rtc <left/right>')

    c = casu.Casu(sys.argv[1])

    myside = sys.argv[2]
    print('My side is {0}'.format(myside))

    # Hardcode thresholds quick'n dirty
    thresholds = {}
    thresholds['left'] = [11300,14500,18500,17600,18500,12000]
    thresholds['right'] = [13500,12400,20000,12500,15500,13500]

    while True:
        
        # Estimate bee density
        detected = [x > t for (x,t) in zip(c.get_ir_raw_value(casu.ARRAY),
                                           thresholds[myside])]
        estimate = float(sum(detected))/float(len(detected))
        c.send_message('cats',str(estimate))

        time.sleep(1.0)
        
