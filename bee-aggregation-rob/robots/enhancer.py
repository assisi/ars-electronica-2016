#!/usr/bin/env python
# -*- coding: utf-8 -*-

#{{{ notes
'''
Controller class for a CASU that emits heat in proportion to local
measure of bees nearby.
- use a plugin for the bee density estimation
- writes messages to custom logfile
- expects two positional arguments:
    (1) the casu rtc file
    (2) the casu custom configuration (a yaml file)

'''

'''
Basic overview of program:

A. setup
  1. open logfiles
  2. interactions with other casus
  3. calibration

B. runtime
  I--update information
  1. read own local values of bees [whatever the source]
  2. receive updates from neighbours
  3. compute running averages (/re-compute)
  4. transmit values to each neighbour

  II--update outputs
  5. re-compute emission temp
  6. compute color to match the emission temp
  7. set temp, set LEDs
  8. log entry for this timestep

C. closedown
  1. close logfile(s)


'''
#}}}

import calibration
import interactions
from assisipy import casu
import argparse, os
import pygraphviz as pgv
import yaml
import time, datetime
#import loggers as loglib
import numpy as np
import os

ERR = '\033[41m'
BLU = '\033[34m'
ENDC = '\033[0m'

_states = {
        1: 'STATE_FIXED_TEMP',
        2: 'STATE_INIT_NOHEAT', 
        3: 'STATE_HEAT_PROPTO', 
        }

STATE_FIXED_TEMP  = 1
STATE_INIT_NOHEAT = 2
STATE_HEAT_PROPTO = 3


#{{{ debug funcs
def disp_vector(vec, fmt=".1f", l=10):
    v = min(len(vec), l)
    s = ", ".join([ "{:{fmt}}".format(_s, fmt=fmt) for _s in vec[0:v] ])
    return s

#}}}
#{{{ utilities
def push_data_1d(arr, new):
    '''
    shift elements along one, and push new data onto the front.
    (since np.roll does not operate in-place, wrap as a function)

    '''
    # THIS SHOULD OPERATE IN-PLACE!
    # we can't seem to use roll because it requires knowing the variable name
    # ahead of time and thus is not flexible.  MAybe I missed a trick but even
    # attempting to spoof a pass-by-reference doesn't gain flexibility.
    # (http://stackoverflow.com/a/986145)
    # so instead, rely on the array mechanism and reassign parts.
    l = len(arr)
    arr[1:] = arr[0:l-1]
    arr[0] = new

    #arr = np.roll(arr, 1)
    #arr[0] = new
#}}}

class Enhancer(object):

    #{{{ class-level defaults for externally-set params
    MANUAL_CALIB_OVERRIDE = False # if leaving the bees in arena, while developing
    DEV_VERB = 5
    DEV_VERB_deep = 0
    VERB = 1

    AVG_HIST_LEN = 60
    HIST_LEN = 80
    MAX_MSG_AGE = 20
    MAX_SENSORS = 6.0
    SELF_WEIGHT = 1.0
    MIN_TEMP = 28.0
    MAX_TEMP = 36.0

    ENABLE_TEMP = True
    REF_DEVIATE = 0.5
    MAIN_LOOP_INTERVAL = 0.2

    SYNCFLASH = True
    SYNC_INTERVAL = 20.0

    INIT_NOHEAT_PERIOD_MINS  = 0.0
    INIT_FIXHEAT_PERIOD_MINS = 0.0
    INIT_FIXHEAT_TEMP        = 28.0
    #}}}

    #{{{ initialiser
    def __init__(self, casu_name, logpath, nbg_file,
                 conf_file=None, calib_conf=None):
        # variables
        self.ts = 0
        self.state = STATE_INIT_NOHEAT
        self.old_state = 0 # set different to above so initial state is always logged
        # read ext config file

        # run or read sensor calibration

        self._rtc_pth, self._rtc_fname = os.path.split(casu_name)
        if self._rtc_fname.endswith('.rtc'):
            self.name = self._rtc_fname[:-4]
        else:
            self.name = self._rtc_fname

        # read all the configuration
        self.parse_conf(conf_file)

        # derived params/variables.
        self.T_RANGE  = self.MAX_TEMP - self.MIN_TEMP
        if self.AVG_HIST_LEN > self.HIST_LEN:
            print "[W] AVG_HIST_LEN of {} does not fit into {} len buffers, increaseing".format(self.AVG_HIST_LEN, self.HIST_LEN)
            self.HIST_LEN = self.AVG_HIST_LEN

        # set up logging
        self.logpath = logpath
        if self.logpath is None:
            self.logpath = "."

        # network - find the interactions
        g_hier = pgv.AGraph(nbg_file)
        g_flat = interactions.flatten_AGraph(g_hier)

        self.in_map = interactions.get_inmap(g_flat, self.name)
        self.out_map = interactions.get_outmap(g_flat, self.name)

        self.most_recent_rx = {}
        for neigh in self.in_map:
            self.most_recent_rx[neigh] = { 'when' : self.ts, 'count': 0.0, 'tomem': False}


        # run calibration procedure
        self.calibrator = calibration.CalibrateSensors(casu_name=self.name,
                                                       logname="temp_calib_log",
                                                       conf_file=calib_conf)
        self.calibrator.calibrate()
        self.calibrator.write_levels_to_file()
        self.calib_data = dict(self.calibrator.calib_data)
        if self.MANUAL_CALIB_OVERRIDE:
            for i in xrange(len(self.calib_data['IR'])):
                self.calib_data['IR'][i] = 700.0

        if self.verb > 1:
            print "[I]{} we have IR calib thresholds of".format(self.name)
            print "[" + ",".join("{:.1f}".format(elem) for elem in self.calib_data['IR']) + "]"


        # now attach to the casu device.
        # (we already attaced in the calib stage.)
        self._casu = self.calibrator._casu
        #self._casu = casu.Casu(rtc_file_name=os.path.join(self._rtc_pth, self.name + ".rtc"))
        _logtime= time.strftime("%H:%M:%S-%Z", time.gmtime())
        self.logfile = os.path.join(
            self.logpath, "{}-{}.log".format(self.name, _logtime))
        self.setup_logger(append=False, delimiter=';')

        # sync flashing log
        if self.SYNCFLASH:
            fn_synclog = '{}/{}-{}.sync.log'.format(self.logpath, self.name, _logtime)
            self.synclog = open(fn_synclog, 'w', 0)
            self.synclog.write("# started at {}\n".format(time.time()))
            self.sync_cnt = 0
            self.last_synchflash_time = time.time()


        # data
        self.bee_hist = {} # record the bee counts from each relevant source
        self.bee_hist['self'] = np.zeros(self.HIST_LEN,)
        for neigh in self.in_map :
            self.bee_hist[neigh] = np.zeros(self.HIST_LEN,)

        self.smoothed_bee_hist = {}  # keep track of time-averaged data
        self.smoothed_bee_hist['self'] = 0.0
        for neigh in self.in_map : self.smoothed_bee_hist[neigh] = 0.0

        self.state_contribs = {}     # keep track of contributions to temp
        self.state_contribs['self'] = 0.0
        for neigh in self.in_map : self.state_contribs[neigh] = 0.0

        # more heat parameters
        #
        self._active_peliter   = False
        self.current_temp      = 28.0
        self.prev_temp         = 28.0
        self.last_temp_update_time  = time.time()
        #

        # timing
        self.init_upd_time = time.time()
        self.__stopped = False

    #}}}

    #{{{ logging
    def setup_logger(self, append=False, delimiter=';'):
        mode = 'a' if append else 'w'
        self._log_LINE_END = os.linesep # platform-independent line endings
        self._log_delimiter = delimiter
        try:
            #self.log_fh = open(self.logfile, mode, 0) # 3rd value is buflen =wrote immediately.
            self.log_fh = open(self.logfile, mode)
        except IOError as e:
            print "[F] cannot open logfile ({})".format(e)
            raise

        pass
    def write_logline(self, ty=None, suffix=''):
        ''' 
        add a line to the logfile, forom various different types - 
        - temperatures (measured, next setpoint, current setpoint)
        - IR sensors (measured plus super-threshold)
        - processed IR data (current value, avg etc)
        - contribution to heating state (avg data of self plus neighbours, sum)
        
        different loglines contain differet components but they all use
        ty;timestamp;<readings...;><nl>

        any reading can have a suffix appended/

        '''
        now = time.time()

        fields = []
        if ty == "IR":
            fields += ["ir_array", now]
            ir_levels = np.array(self._casu.get_ir_raw_value(casu.ARRAY))[0:6]
            fields += ir_levels

        elif ty == "HEAT":
            fields += ["temperatures", now]
            for sensor in [casu.TEMP_L, casu.TEMP_R, casu.TEMP_B, casu.TEMP_F,]:
                #casu.TEMP_WAX, casu.TEMP_CASU]:
                _t = self._casu.get_temp(sensor)
                fields.append(_t)

            _sp, onoff = self._casu.get_peltier_setpoint()
            fields.append(_sp)
            fields.append(int(onoff))
        elif ty == "MODE":
            # lets write this manualy for more flexibility
            fields += ['state', now, self.state, _states[self.state]]
        
        elif ty == "HEAT_CALCS":
            # most of the info comes from the suffix.
            fields += ["heat_calcs", now]

        elif ty == "NH_DATA":
            fields += ["nh_data", now]

        # elif ...

        s = self._log_delimiter.join([str(f) for f in fields])
        if len(suffix):
            s += self._log_delimiter + suffix
        s += self._log_LINE_END
        self.log_fh.write(s)
        self.log_fh.flush()


    def _cleanup_log(self):
        self.log_fh.close()
        print "[I] finished logging to {}.".format(self.logfile)
        pass



    #}}}

    #{{{ parse external config
    def parse_conf(self, conf_file):
        self._ext_conf = {}
        if conf_file is not None:
            with open(conf_file) as f:
                self._ext_conf = yaml.safe_load(f)

        for var in [
                'MANUAL_CALIB_OVERRIDE',
                'DEV_VERB',
                'DEV_VERB_deep',
                'VERB',
                'AVG_HIST_LEN',
                'HIST_LEN',
                'MAX_MSG_AGE',
                'MAX_SENSORS',
                'SELF_WEIGHT',
                'MIN_TEMP',
                'MAX_TEMP',
                'ENABLE_TEMP',
                'REF_DEVIATE',
                'MAIN_LOOP_INTERVAL',
                'SYNCFLASH',
                'SYNC_INTERVAL',
                'INIT_NOHEAT_PERIOD_MINS',
                'INIT_FIXHEAT_PERIOD_MINS',
                'INIT_FIXHEAT_TEMP',
                ]:

            defval = getattr(self, var)
            confval = self._ext_conf.get(var, None)
            if confval is not None:
                print "[DD] setting {} from file to {} (default={})".format(
                        var, confval, defval)

                setattr(self, var, confval)
            else:
                setattr(self, var, defval) # probably unecessary - verify

        self.verb = self.VERB
    #}}}

    #{{{ read sensors
    def measure_ir_sensors(self):
        ir_levels = np.array(self._casu.get_ir_raw_value(casu.ARRAY))
        count = 0

        # need to ignore the last one because it should not be used
        for i, (val, t) in enumerate(zip(ir_levels, self.calib_data['IR'])):
            if i < 6: # ignore last one
                if (val > t): count += 1

        self.current_count = float(count / self.MAX_SENSORS)

    def get_actual_temp(self):
        _T = []
        for sensor in [casu.TEMP_L, casu.TEMP_R, casu.TEMP_B, casu.TEMP_F]:
            _t = self._casu.get_temp(sensor)
            if _t > 2.0 and _t < 50.0: # value is probably ok
                _T.append(_t)
        if len(_T):
            T = np.array(_T)
            return T.mean()
        else:
            return -1.0
    #}}}

    #{{{ modulate actuators
    def set_fixed_temp(self, temp=None):
        '''
        set to a target temperature `temp`. If no argument provided, the value
        used is `self.INIT_FIXHEAT_TEMP`
        '''
        # if we have a fixed temperature, we assert it, or check it is already
        # asserted.
        # check temp is ok
        if temp is None:
            target_temp = self.INIT_FIXHEAT_TEMP
        else:
            target_temp = temp
        _tref, _on = self._casu.get_peltier_setpoint()
        if _tref == target_temp and _on is True:
            # nothing to do
            pass
        else:
            self._casu.set_temp(target_temp)
            self.current_temp = target_temp
            self._active_peliter = True
            # update the info on it
            now = time.time()
            self.last_temp_update_time = now
            tstr = time.strftime("%H:%M:%S-%Z", time.gmtime())
            print "[I][{}] requested new fixed temp @{} from {:.2f} to {:.2f}".format(
                    self.name, tstr, self.prev_temp, self.current_temp)
            self.prev_temp = self.current_temp

    def unset_temp(self):
        self._casu.temp_standby()
        self._active_peliter = False


    def update_temp_wrapper(self):
        '''
        only change the temperture when we requested somehting far enough
        away that it will make a difference.  This is only to work around
        the frequency of setting new values.

        '''
        if abs(self.current_temp - self.prev_temp) > self.REF_DEVIATE:
            # make a new request
            t = self.get_actual_temp()

            #if abs(self.current_temp - self.MIN_TEMP) < 0.25 and abs(t - self.current_temp) <= 0.5
            if abs(t - self.current_temp) <= 0.5:
                # turn off the peltier, don't heat at this level.
                self._casu.temp_standby()
                self._active_peliter = False
                print "[DD]{}|{} cancelled peltier because low request {:.2f}oC (andalready close: {:.2f}oC)".format(
                        self.name, self.ts, self.current_temp, t)
            else:
                self._casu.set_temp(self.current_temp)
                self._active_peliter = True
                print "[DD]{}|{} set peltier to temp {}oC".format(
                        self.name, self.ts, self.current_temp)
            # update the info on it
            now = time.time()
            self.last_temp_update_time = now
            tstr = time.strftime("%H:%M:%S-%Z", time.gmtime())
            print "[I][{}] requested new temp @{} from {:.2f} to {:.2f}".format(
                    self.name, tstr, self.prev_temp, self.current_temp)
            self.prev_temp = self.current_temp
    #}}}

    #{{{ comms
    def tx_count(self, dest):
            #self.smoothed_bee_hist[neigh] = vd.mean()
        #s = "{}".format(self.current_count)
        s = "{:.3f}".format(self.smoothed_bee_hist['self'])
        if self.verb > 2:
            print "\t[i]==> {} send msg ({} by): '{}' bees, to {}".format(
                self.name, len(s), s, dest)

        self._casu.send_message(dest, s)

    def recv_all_incoming(self, retry_cnt=0):
        msgs = {}
        try_cnt = 0
        while True:
            msg = self._casu.read_message()

            if msg:
                txt = msg['data'].strip()
                src = msg['sender']
                nb =  float(txt.split()[0])
                msgs[src] = nb

                if self.verb > 1:
                    print "\t[i]<== {3} recv msg ({2} by): '{1}' bees, {4} from {0} {5}".format(
                        msg['sender'], nb, len(msg['data']), self.name, BLU, ENDC)
            else:
                # buffer emptied, return
                try_cnt += 1
                if try_cnt > retry_cnt:
                    break

        return msgs

    #}}}

    def update_interactions(self):
        # read incoming messages
        neigh_cnts = self.recv_all_incoming()
        # update buffers for all neighbours
        for src, count in neigh_cnts.items():
            if src in self.most_recent_rx:
                self.most_recent_rx[src]['when']  = self.ts
                self.most_recent_rx[src]['count'] = float(count)
                self.most_recent_rx[src]['tomem'] = False
            else:
                print "[W] recv data from {}, unexpectedly".format(src)

        # we will base the contributions on the smoothed / processed data, not
        # most recent.


    #{{{ update_averages
    def update_averages(self):
        # if we have new data for a given neighbour (upstream), then
        for neigh, data in self.most_recent_rx.items():
            if data['tomem'] is False and (self.ts - data['when']) < self.MAX_MSG_AGE:
                if 0: print "D] in here! pusing data {} to {} [{}]".format(
                        data['count'], neigh, self.name)
                push_data_1d(self.bee_hist[neigh], data['count'] )
                data['tomem'] = True
            else:
                if data['tomem'] is False:
                    # we must be with out of date info. Emit a message
                    print "W]{} old info (data from {}; now:{} => age={}, thr {} [already transferred? {}])".format(
                        self.name, data['when'], self.ts,
                        self.ts - data['when'],
                        self.MAX_MSG_AGE,
                        data['tomem']
                    )

        # we always have an update for self, so put that in too.
        #print "[D1] {} pre ".format(self.name), disp_vector(self.bee_hist['self'])
        push_data_1d(self.bee_hist['self'], self.current_count)

        #self.bee_hist['self'] = np.roll(self.bee_hist['self'], 1)
        #self.bee_hist['self'][0] = self.current_count
        #print "[D1] {} post".format(self.name), disp_vector(self.bee_hist['self'])

        valid = min(self.ts, self.AVG_HIST_LEN)
        #v2 = min(self.ts, 10) # visualisation (debug)

        for neigh in self.bee_hist: # including 'self' here
            vd = np.array(self.bee_hist[neigh][0:valid])
            #print "[DD] ", neigh, self.bee_hist[neigh][0:valid].shape, valid
            if self.DEV_VERB_deep:
                if neigh == 'self':
                    _v = self.current_count
                else:
                    _v = self.most_recent_rx[neigh]['count']

                print "[DD] - {:14} <--{:14} new val {:.1f} (avg:{:.1f})".format(
                    self.name, neigh, _v, self.smoothed_bee_hist[neigh]),
                print disp_vector(vd, l=10)
            #print ", ".join(["{:.1f}".format(_s) for _s in vd[0:v2] ])
            self.smoothed_bee_hist[neigh] = vd.mean()


        self.state_contribs = {}     # keep track of contributions to temp


        for neigh in self.smoothed_bee_hist: # including 'self' here
            if neigh == 'self':
                w = self.SELF_WEIGHT
            else:
                w = self.in_map[neigh].get('w')

            v = self.smoothed_bee_hist[neigh]
            self.state_contribs[neigh] = w * v



    #}}}


    #{{{ top-level actions
    def one_cycle(self):
        self.ts += 1
        self.update_info()

        self.update_outputs()

        self.sync_flash()
    #}}}


    #{{{ update_info
    def update_info(self):

        #B. runtime
        #  I--update information
        #  1. read own local values of bees [whatever the source]
        self.measure_ir_sensors()

        #  2. receive updates from neighbours
        self.update_interactions()

        #  3. compute running averages (/re-compute)
        self.update_averages()
        if self.DEV_VERB:
            print "[D]({}) {}. {:.2f} ({:.0f} sensors) [{:.2f} avg]".format(
                self.name, self.ts, self.current_count,
                self.current_count * self.MAX_SENSORS,
                self.smoothed_bee_hist['self']
            )
            #for neigh in self.smoothed_bee_hist.keys():
            #    print "\t{:14}: {:.2f} {:+.2f}".format(
            #        neigh, self.smoothed_bee_hist[neigh],
            #        self.state_contribs[neigh])

        #  4. transmit values to each neighbour
        for phys_dest, linkname in self.out_map.iteritems():
            # link name is the destination for msg transmission
            self.tx_count(linkname)
    #}}}

    #{{{ update_outputs
    def update_outputs(self):
        # we always recompute what to do, but don't always do it.
        #II--update outputs
        #5. re-compute emission temp
        activation_level = 0.0
        _nh_fields = [ len(self.smoothed_bee_hist.keys()), ]
        for neigh in self.smoothed_bee_hist.keys():
            if neigh == 'self':
                w = self.SELF_WEIGHT
            else:
                w = self.in_map[neigh].get('w')

            #activation_level += self.smoothed_bee_hist[neigh]
            activation_level += self.state_contribs[neigh]
            print "\t{:14}: {:.2f} {:+.2f} | w={:+.2f}".format(
                neigh, self.smoothed_bee_hist[neigh],
                self.state_contribs[neigh], w)
            # to write the neighbour weight data, it requires a bit of work because 
            # each casu has a different length data. So we will need a specific parser I suppose
            #>>>ty, time, num_neigh, <who, weight, raw, contrib, > for each edge. <<<
            _nh_fields += [neigh, w, self.smoothed_bee_hist[neigh],
                    self.state_contribs[neigh]]
            
        self.write_logline(ty="NH_DATA", suffix=
                self._log_delimiter.join([str(f) for f in _nh_fields]))

        # clip in [0, 1]
        activation_level = sorted([0.0, activation_level, 1.0])[1]
        # multiply by range
        bonus = self.T_RANGE * activation_level
        self.current_temp = self.MIN_TEMP + bonus
        print "\t======{:4}========  {:.1f}% ({:.2f}oC) ==> {:.2f}|{:.2f}oC [{}]".format(
            self.ts, activation_level * 100.0, bonus, self._casu.get_temp(casu.TEMP_L), self._casu.get_temp(casu.TEMP_R),
            self.name)
        _fields = [activation_level, bonus, ]
        self.write_logline(ty="HEAT_CALCS", 
                suffix=self._log_delimiter.join([str(f) for f in _fields]) )


        # if initial quiescent period has passed, allow LEDs and heaters on.
         
        # record to logfile on cycles when it transitions
        now = time.time()
        elap = now - self.init_upd_time
        # IF IN FIXED TEMP, -> 1/2 blue
        if elap < (self.INIT_FIXHEAT_PERIOD_MINS * 60.0):
            # set the CASU to a fixed temperature.
            #self._casu.set_diagnostic_led_rgb(b=0.5) # NOTE: REMOVED since misleading
            self.state = STATE_FIXED_TEMP

            if self.ENABLE_TEMP:
                self.set_fixed_temp()
            if self.DEV_VERB:
                print "[DD3] temp fixed heat, equalise arena. ({:.1f}s remain)".format(
                         (self.INIT_FIXHEAT_PERIOD_MINS * 60.0) - elap)

        # IF IN MAIN MODE, SET RED PROPTO TEMP
        elif elap > (self.INIT_NOHEAT_PERIOD_MINS * 60.0):
            self.state = STATE_HEAT_PROPTO
            #6. compute color to match the emission temp
            #   (just propto range of temp)
            self._casu.set_diagnostic_led_rgb(r=activation_level, g=0, b=0)
            #7. set temp, set LEDs
            if self.ENABLE_TEMP:
                    self.update_temp_wrapper() # don't change rq if small
        # if in DEBUG NO HEAT MODE, SET TO DK GREY.
        else:
            self.state = STATE_INIT_NOHEAT
            self._casu.set_diagnostic_led_rgb(r=0.2, g=0.2, b=0.2)
            if self.DEV_VERB:
                print "[DD2] temp no heat, free bee movement. ({:.1f}s remain)".format(
                         (self.INIT_NOHEAT_PERIOD_MINS * 60.0) - elap)
            #self._casu.set_diagnostic_led_rgb(g=activation_level+0.33, r=0, b=0)

        #8. log entry for this timestep

        # compute whether state change needs log entry
        if self.state != self.old_state:
            self.write_logline(ty="MODE")
        self.old_state = self.state

        # always write logs for sensor values
        self.write_logline(ty='IR')
        self.write_logline(ty='HEAT')


        #TODO:!!!!
    #}}}

    #{{{ emit flash pulse to help align videos
    def sync_flash(self):
        if self.SYNCFLASH:
            now = time.time()
            if now - self.last_synchflash_time > self.SYNC_INTERVAL:
                # update time (sync to beginning of flash)
                self.last_synchflash_time = now
                # record pre
                s = "{}; {}; {};".format(now, self.sync_cnt, "start")
                self.synclog.write(s + "\n")
                self.synclog.flush()
                print "[D] synch {}".format(s)
                # FLASH
                self._sync_flash()
                # RECORD post
                now2 = time.time()
                self.synclog.write("{}; {}; {}; \n".format(now2, self.sync_cnt, "end"))
                self.sync_cnt += 1

    def _sync_flash(self, dur_mult=1.0, log=True):
        '''
        by default a 0.4s cycle of R/G/B (blocking).
        Increase duration by setting dur_mult >1
        '''
        # read current state
        rgb = self._casu.get_diagnostic_led_rgb()
        # TODO: insert log line before switching on.
        self._casu.set_diagnostic_led_rgb(r=1.0)
        time.sleep(0.05)
        self._casu.set_diagnostic_led_rgb()
        time.sleep(0.05)

        self._casu.set_diagnostic_led_rgb(g=1.0)
        time.sleep(0.10)
        self._casu.set_diagnostic_led_rgb()
        time.sleep(0.05)

        self._casu.set_diagnostic_led_rgb(b=1.0)
        time.sleep(0.15)
        self._casu.set_diagnostic_led_rgb()
        time.sleep(0.05)
        # TODO: insert log line after end

        # put back original state
        self._casu.set_diagnostic_led_rgb(*rgb)
    #}}}

    #{{{ stop
    def stop(self):
        self._casu.set_diagnostic_led_rgb(0.2, 0.2, 0.2)
        if not self.__stopped:
            s = "# {} Finished at: {}".format(
                self.name, datetime.datetime.fromtimestamp(time.time()))
            self.log_fh.write(s + "\n")
            self._cleanup_log()
            self._casu.stop()
            self.__stopped = True
    #}}}



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('name', )
    parser.add_argument('-c', '--conf', type=str, default=None)
    parser.add_argument('--nbg', type=str, default=None)
    parser.add_argument('-o', '--output', type=str, default=None)
    args = parser.parse_args()

    c = Enhancer(args.name, nbg_file=args.nbg, logpath=args.output, conf_file=args.conf)

    if c.verb > 0: print "Bifurcation Enhancer - connected to {}".format(c.name)
    try:
        while True:
            time.sleep(c.MAIN_LOOP_INTERVAL)
            c.one_cycle()
    except KeyboardInterrupt:
        print "shutting down casu {}".format(c.name)
        c.stop()

    c.stop()
    if c.verb > 0: print "Bifurcation Enhancer {} - done".format(c.name)



