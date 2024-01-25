TP/PRO/BV-43 Fragmented Data Transmission with a Retransmission (apscMaxWindowSize is 3)

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
2    gZR2 joins gZR1   
3    Set apscMaxWindowSize is equal to 3 by some means.   
4    Set apsInterframeDelay is equal to 100 milliseconds 
by some means. 
5    Set the DUT ZC does not issue transmit counted packet 
with block number 01 over the air by some means. 

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
1)	DUT ZC issues first transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say0x1, 0x2, 0x3….0x1E)).
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled. 
3)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b01
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 08.
5)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and issues second transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
6)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
7)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
8)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
9)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and issues third transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
13)	Upon unicast from DUT ZC, gZR2 shall send an altered APS ACK to DUT ZC via gZR1.The APS ACK shall be altered to provide the values listed in the following 3 steps.
14)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
15)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFD.
16)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 0.
17)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and retransmit second transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
18)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
19)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
20)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
21)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
22)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
23)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
24)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 0.
25)	DUT ZC waits for apscAckWaitDuration and issues fourth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78).
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled
27)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
29)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and issues fifth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x79, 0x7A, 0x7B……….0x96). 
30)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
31)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
32)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 04.
33)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) issues sixth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
34)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
35)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
36)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 05.
37)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
38)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
39)	gZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame shall be 0xFF.
40)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
41)	DUT ZC waits for apscAckWaitDuration and issues seventh transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 06.
45)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and issues eighth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
46)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
47)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
48)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 07.
49)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
50)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 0.
51)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
52)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 06.
Fail verdict:
1)	DUT ZC does not issue first transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 1E)
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled. 
3)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame  is not b01
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame is not 08.
5)	DUT ZC does not wait for apsInterframeDelay (say 100 milliseconds) and does not issue a second transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
6)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
7)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame is not b10
8)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame is not 01.
9)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue third transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not  b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 02.
13)	Upon unicast from DUT ZC, gZR2 does not send an altered APS ACK to DUT ZC via gZR1.The APS ACK is not altered to provide the values listed in the following 3 steps.
14)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
15)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFD.
16)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 0.
17)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and does not retransmit second transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
18)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
19)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not  b10
20)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
21)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
22)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
23)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
24)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 0.
25)	DUT ZC waits for apscAckWaitDuration and does not issue fourth transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78)
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled
27)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 03.
29)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue fifth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say  0x79, 0x7A, 0x7B……….0x96). 
30)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
31)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
32)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 04.
33)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) does not issue sixth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
34)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
35)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
36)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 05.
37)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
38)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
39)	gZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame does not 0xFF.
40)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 03.
41)	DUT ZC waits for apscAckWaitDuration and does not issue seventh transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 06.
45)	DUT ZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue eighth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
46)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
47)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
48)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 07.
49)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
50)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
51)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
52)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 06.

