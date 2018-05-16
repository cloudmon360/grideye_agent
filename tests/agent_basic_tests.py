import time
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

    @classmethod
    def teardown_class(cls):
        Tests.proc.kill()

    def test_agent_started(self):
        assert(Tests.proc.getproc() != None)
        
