import os
import sys
import time
here = sys.path[0]
print here
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import signal

#WHISPER MOTE
MOTE_IP = 'bbbb::1415:92cc:0:5'

c = coap.coap()

#primitive 1 test

# toggle the parent of 4 from 2 to 3
p = c.PUT(
    'coap://[{0}]:5683/w'.format(MOTE_IP),
    payload = [ord('x'),4,2,3],
)
time.sleep(3)
c.close()

