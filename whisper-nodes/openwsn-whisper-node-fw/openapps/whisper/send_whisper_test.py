import os
import sys
import time
here = sys.path[0]
print here
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import signal

#WHISPER OPENMOTE
MOTE_IP = 'bbbb::0012:4b00:0613:0f07'

c = coap.coap(udpPort=61618)

#Test 2, primitive 1.

# trigger the parent switch in 4, from 2 to parent 3
p = c.PUT(
    'coap://[{0}]:5683/w'.format(MOTE_IP),
    payload = [ord('x'),0x67,0x61,0x2b],
)
print "OK"
time.sleep(3)
c.close()

