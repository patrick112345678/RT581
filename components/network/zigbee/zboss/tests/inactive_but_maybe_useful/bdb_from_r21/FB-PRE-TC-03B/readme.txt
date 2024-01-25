5.1.5 FB-PRE-TC-03B: Service discovery - client side additional tests
This test contains additional checks for handling of the service discovery responses, if defined.


Required devices:
DUT - Device operational on a network: ZC, ZR, or ZED; 
supporting generation of service discovery request primitives

THr1 - TH ZR operational on a network; 
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.

THe1 - TH Sleeping ZED,
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P0 - THr1, DUT and THe1 are factory new and off.
P1 - THr1 and DUT are turned on. THr1 and DUT become operational on the same network (bdbNodeIsOnANetwork = TRUE), formed by one of them. 
P3 - THe1 joins the network at THr1 (bdbNodeIsOnANetwork = TRUE).
P4 - Make sure the Binding table of the DUT is empty. 
If DUT uses groupcast binding, make sure the Groups table of THr1 and THe1 is empty.


Additional info:
 - DUT starts f*&b as initiator after DUT_FB_FIRST_START_DELAY seconds, then waiting for f&b complete by timeout (FB_DURATION) and restart f&b after DUT_FB_START_DELAY seconds (in cycle).
 - THr1 starts f&b as target after THR1_FB_START_DELAY seconds and retriggers it after THR1_FB_START_DELAY seconds every time when f&b completes by timeout.
 - THe1 start f&b as target after THE1_FB_FIRST_START_DELAY seconds for FB_DURATION seconds.
   After f&b duration expires the1 restart f&b in THE1_FB_START_DELAY seconds
