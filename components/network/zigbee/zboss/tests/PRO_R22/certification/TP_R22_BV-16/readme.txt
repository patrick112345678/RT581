TP/R22/BV-16 Network Broadcast from DUT Router

Objective:
Broadcast from DUT ZR1, going through two the following scenarios, 
a)	DUT must relay broadcasts sent by another device and 
b)	Verify lack of passive acknowledgement triggers router to re-broadcast. Verify timeout (9 seconds). 
c)	DUT must relay broadcasts sent by another device immediately after reboot
d)	DUT must not relay its own broadcast after reboot. and DUT must not trigger address conflict immediately after reboot. 
e)	Verify lack of passive acknowledgement triggers router to re-broadcast. Verify timeout (9 seconds). Send a new broadcast after timeout with the same parameters as the old broadcast.that triggers address conflict 

Initial Conditions:
	gZC
     |
  DUT ZR1
     |
   gZR2
   
gZC
PANid= 0x1AAA
Logical Address = 0x0000
0x aa aa aa aa aa aa aa aa

DUT ZR1	
PANid= 0x1AAA
0x 00 00 00 01 00 00 00 00

gZR2
PANid= 0x1AAA
0x 00 00 00 09 00 00 00 01

Test Procedure:
TP 1. gZR2 issues a Message Buffer Test Request with NWK address set at 0xffff with passive acknowledgement turn off on all devices.
This covers objective A and B.
TP 2. Reboot DUT ZR1
gZR2 issues a Message Buffer Test Request with NWK address set at 0xffff with passive acknowledgement turn off on all devices. 
This covers Objective C.
TP 3. Reboot DUT ZR1
gZR2 issues a broadcast Message Buffer Test Request with NWK Destination address set to 0xffff and the NWK Source Address set to that of DUT ZR1 (a spoofed message). 
This Covers Objective D.
TP 4. Wait nwkBroadcastDeliveryTime (9 seconds).
gZR2 issues a broadcast Message Buffer Test Request with NWK Destination address set to 0xffff and the NWK Source Address set to that of DUT ZR1 (a spoofed message). 
This covers objective E.

Pass Verdict:
EO 1. DUT ZR1 rebroadcasts Message Buffer Test Request 2 times (3 total transmissions)
EO 2. 2DUT ZR1 rebroadcasts Message Buffer Test Request 2 times (3 total transmissions)
EO 3. DUT ZR1 shall rebroadcast Message buffer test request after reboot
EO 4. Upon repowering DUT ZR1 and receiving the broadcast, DUT ZR1 SHALL NOT trigger an address conflict (Network Status Command with status 0x0D).
EO 5. Upon receiving the broadcast, DUT ZR1 SHALL trigger an address conflict by sending a broadcast Network Status Command with status code 0x0D.

Fail Verdict:
1) DUT ZR1 doesn’t received retransmit the broadcast message
2) DUT ZR1 doesn’t rebroadcast routes for 9 secondsretransmit the broadcast message
3) DUT ZR1 doesn’t rebroadcast routes message to gZC for 1 second 
4) DUT ZR1 doesn’t  rebroadcast routes to gZC after power up after 15 seconds
5) DUT ZR1 triggers an address conflict by sending a Network Status Command with Status 0x0D.
6) DUT ZR1 does NOT trigger an address by sending a Network Status Command with Status 0x0D.
