import time
import helpers
import pytest
import re

from subprocess import *

class Proc():
    def __init__(self):
        self.proc = None

    def start(self, proc):
        self.proc = proc
        
    def kill(self):
        self.proc.kill()

    def read(self):
        return self.proc.stderr.read(1000)

    def getproc(self):
        return self.proc
    
class Tests():
    proc = Proc()

    @classmethod
    def setup_class(cls):
        agent_args = ['./grideye_agent',
                      '-F',
                      '-k', 'pidfile.pid']
        
        Tests.proc.start(Popen(agent_args, stdout=PIPE, stderr=PIPE))

        assert(Tests.proc.getproc() != None)

    @classmethod
    def teardown_class(cls):
        Tests.proc.kill()
        
    def test_agent_plugins(self):
        agent_plugins = ['grideye_http.so.1',
                         'grideye_mem_read.so.1',
                         'grideye_sysinfo.so.1',
                         'grideye_cycles.so.1',
                         'grideye_wlan.so.1',
                         'grideye_dhrystones.so.1',
                         'grideye_diskio_write.so.1',
                         'grideye_diskio_write_rnd.so.1',
                         'grideye_diskio_read.so.1']

        buf = Tests.proc.read()
        for plugin in agent_plugins:
            match =  re.match('.*' + plugin + '.*', str(buf))
            assert(match != None)
