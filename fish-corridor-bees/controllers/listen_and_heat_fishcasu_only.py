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

    # Hardcode thresholds quick'n dirty
    thresholds = {}
    thresholds['left'] = [11300,14500,18500,17600,18500,12000]
    thresholds['right'] = [15000,13500,20500,12500,15500,13500]

    directions = {'left':'CCW','right':'CW'}
    mydirection = directions[myside]

    print('My side is {0}, my direction is {1}'.format(myside,mydirection))

    while True:
        msg = c.read_message()
        if msg:
            print('Received: ' + msg['data'])
            data = msg['data'].split(',')
            count = 0
            for item in data:
                (fish,direction) = item.split(':')
                if fish == 'fishCasu':
                    if direction == mydirection:
                        # CASUs have decided for our side -> heat up
                        temp_set = sorted([temp_min,temp_set+0.5,temp_max])[1]
                        c.set_temp(temp_set)
                        c.set_diagnostic_led_rgb(r = 1)
                    else:
                        # Cool down
                        temp_set = sorted([temp_min,temp_set-0.5,temp_max])[1]
                        c.set_temp(temp_set)
                        c.set_diagnostic_led_rgb(b=1)

        # Estimate bee density
        detected = [x > t for (x,t) in zip(c.get_ir_raw_value(casu.ARRAY),
                                           thresholds[myside])]
        estimate = float(sum(detected))/float(len(detected))
        c.send_message('cats',str(estimate))

        time.sleep(0.5)
