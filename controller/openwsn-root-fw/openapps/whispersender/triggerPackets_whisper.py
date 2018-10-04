import os
import sys
import time
here = sys.path[0]
print here
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import signal

#WHISPER OPENMOTE
MOTE_IP = 'bbbb::0012:4b00:0613:0636'

c = coap.coap(udpPort=61618)

p = c.PUT(
    'coap://[{0}]:5683/p'.format(MOTE_IP),
    payload = [ord('x')],
)
print "OK"
time.sleep(3)
c.close()

