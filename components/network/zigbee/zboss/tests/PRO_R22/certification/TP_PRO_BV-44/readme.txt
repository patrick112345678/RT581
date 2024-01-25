TP/PRO/BV-44 Fragmented Data Transmission with Multiple Retransmissions (apscMaxWindowSize is 3)

Verify multiple frames are lost in the network, including a frame which has the 
highest block number in the window is retransmitted. This test case is applicable to 
1both Sub-GHz and 2.4 GHz interfaces.

Initial Conditions:
The required network set-up for the test harness is as folllows
DUT ZC 
PANid= Generated in a random 
manner(within the range 0x1 to 0x3FFF) 
Logical Address = 0x0000 
IEEE address=0xaa aa aa aa aa aa aa aa

gZR1
Pan id=same PANID of ZC1. 
Logical Address=Generated in a random 
manner(within the range 1 to 0xFFF7) 
IEEE address=0x00 00 00 01 00 00 00 00 

gZR2  Pan id=same PANID  of  ZC1 
Logical Address=Generated in a random 
manner(within the range 1 to 0xFFF7) 
IEEE address=0x00 00 00 02 00 00 00 00

Pre-test steps:
1    gZR1 joins DUT ZC.   
2    gZR1 joins DUT ZR2. Set the fragmentation threshold 
condition as 30 bytes 
3    Set apscMaxWindowSize is equal to 3 by some 
means. 
4    Set apsInterframeDelay is equal to 100milliseconds 
by some means. 
5    After initiating data transmission, set the DUT ZC 
does not issue transmit counted packet with block 
numbers 00 and 02 over the air by some means.

Test Procedure:
1. DUT ZC sends an APSDE-DATA.request to 
gZR2. 
 
APSDE-DATA.Request 
DstAddrMode = 0x02 (direct address) 
DstAddress = ZR2 address 
DstEndpoint=0xF0 
ProfileId,=0x7f01  
ClusterId=0x0001(transmit count packet) 
SrcEndpoint=0x01 
asduLength=0xF0 
asdu=Transmit count with sequence length octet 
TxOptions=0x08 
Radius Counter=0x02

Pass verdict:
1)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and issues second transmit counted packetto DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
3)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
5)	DUT ZC waits for apsFragmentation-RetryTimeoutseconds and retransmit first transmit counted packet to DUT ZR2 via gZR1 with the payload. As per the test profile say 0x1, 0x2, 0x3……….0x1E).
6)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
7)	DUT ZC over the air packet,in the APS header extended frame control field of the extended header sub-frame shall be b01
8)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 08.
9)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and retransmit second transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say0x1F, 0x20, 0x21……. 0x3C).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
13)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and retransmi third transmit counted packetto DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
14)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
15)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
16)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
17)	Upon unicast from DUT ZC, DUT ZR2 sends the APS ACK to DUT ZC via gZR1. 
18)	  DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shallset as 0.
19)	DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
20)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 0.
21)	DUT ZC waits for apscAckWaitDuration and issues fourth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78).
22)	DUT ZC over the air packet,in the APS header extended header sub-frame of the frame control field is enabled
23)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
24)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
25)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and issues fifth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x79, 0x7A, 0x7B……….0x96). 
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
27)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 04.
29)	DUT ZC waits for apsInterframeDelay (say100milliseconds) issues sixth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
30)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
31)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
32)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 05.
33)	Upon unicast from DUT ZC, DUT ZR2 sends the APS ACK to    DUT ZC via gZR1. 
34)	  DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
35)	DUT ZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame shall be 0xFF.
36)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
37)	DUT ZC waits for apscAckWaitDuration and issues seventh transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
38)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
39)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
40)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 06.
41)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and issues eighth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 07.
45)	Upon unicast from DUT ZC, DUT ZR2 sends the APS ACK to    DUT ZC via gZR1. 
46)	DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
47)	DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
48)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 06.
Fail verdict:
1)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and does not issue second transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
3)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
5)	DUT ZC waits for apsFragmentation-RetryTimeoutseconds and does not retransmit first transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say0x1, 0x2, 0x3……….0x 1E).
6)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
7)	DUT ZC over the air packet,in the APS header extended frame control field of the extended header sub-frame does not b01
8)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 08.
9)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and does not retransmit second transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not  b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
13)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and does not retransmi third transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
14)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
15)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
16)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 02.
17)	Upon unicast from DUT ZC, DUT ZR2 does not send the APS ACK to DUT ZC via gZR1. 
18)	  DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
19)	DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
20)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 0.
21)	DUT ZC waits for apscAckWaitDuration and does not issue fourth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78).
22)	DUT ZC over the air packet,in the APS header extended header sub-frame of the frame control field is disabled
23)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
24)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 03.
25)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and does not issue fifth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x79, 0x7A, 0x7B……….0x96). 
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
27)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 04.
29)	DUT ZC waits for apsInterframeDelay (say100milliseconds) does not issue sixth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
30)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
31)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
32)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 05.
33)	Upon unicast from DUT ZC, DUT ZR2 does not send the APS ACK to    DUT ZC via gZR1. 
34)	  DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
35)	DUT ZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame does not 0xFF.
36)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 03.
37)	DUT ZC waits for apscAckWaitDuration and does not issue seventh transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
38)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
39)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
40)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 06.
41)	DUT ZC waits for apsInterframeDelay (say100milliseconds) and does not issue eighth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 07.
45)	Upon unicast from DUT ZC, DUT ZR2 does not send the APS ACK to    DUT ZC via gZR1. 
46)	DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
47)	DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
48)	DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 06.

