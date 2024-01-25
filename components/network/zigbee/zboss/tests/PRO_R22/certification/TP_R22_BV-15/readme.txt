TP/R22/BV-15 transmission with fragmentation  (apscMaxWindowSize is 3)

Verify unicast data transmission with fragmentation. This test case is applicable to
both Sub-GHz and 2.4 GHz interfaces.

Initial Conditions:
		gZC
         |
       gZR1
         |
      DUT ZR2

DUT gZC
PANid=Generated in a random manner (within the range 0x1 to 0x3FFF)
Logical Address = 0x0000
IEEE address=0xaa aa aa aa aa aa aa aa 

gZR1	
PANid=Pan id=same PANID of ZC1.
Logical Address=Generated in a random manner (within the range 1 to 0xFFF7)
IEEE address=0x00 00 00 01 00 00 00 00

DUT  gZR2	
PANid=Pan id=same PANID  of  ZC1
Logical Address=Generated in a random manner (within the range 1 to 0xFFF7)
IEEE address=0x00 00 00 02 00 00 00 00

Test Procedure:
1. DUT gZC sends an APSDE-DATA.request to gZR2DUT ZR2.
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

Pass Verdict:
1)	gZC issues first transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3…..0x1E)
2)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled. 
3)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b01
4)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 08.
5)	gZC waits for apsInterframeDelay (say 100 milliseconds) and issues second transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
6)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
7)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
8)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
9)	gZC waits for apsInterframeDelay (say 100 milliseconds) and issues third transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
10)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
11)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
12)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
13)	Upon unicast from gZC, DUT ZR2 sends the APS ACK with extended APS header present to gZC via gZR1. 
14)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 1.
15)	DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
16)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 0.
17)	gZC waits for apscAckWaitDuration and issues fourth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78).
18)	gZC over the air packet,in the APS header extended header sub-frame of the frame control field is enabled
19)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
20)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
21)	gZC waits for apsInterframeDelay (say 100 milliseconds) and issues fifth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x79, 0x7A, 0x7B……….0x96). 
22)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
23)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
24)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 04.
25)	gZC waits for apsInterframeDelay (say 100 milliseconds) issues sixth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
26)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
27)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
28)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 05.
29)	Upon unicast from gZC, DUT ZR2 sends the APS ACK to    gZC via gZR1. 
30)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 1.
31)	 DUT ZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame shall be 0xFF.
32)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
33)	 gZC waits for apscAckWaitDuration and issues seventh transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
34)	 gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
35)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
36)	 gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 06.
37)	 gZC waits for apsInterframeDelay (say 100 milliseconds) and issues eighth transmit counted packet to  DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
38)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
39)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10.
40)	 gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 07.
41)	Upon unicast from gZC, DUT ZR2 sends the APS ACK to   gZC via gZR1. 
42)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 1.
43)	 DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
44)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 06.

Fail Verdict:
1)	gZC does not issue first transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 1E)
2)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled. 
3)	gZC over the air packet,in the APS header extended frame control field of the extended header sub-frame does not b01
4)	gZC over the air packet, in the APS header block number of the extended header sub-frame does not 08.
5)	gZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue second transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x1F, 0x20, 0x21………. 0x3C).
6)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
7)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
8)	gZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
9)	gZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue third transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x3D, 0x3E, 0x3F……..0x5A).
10)	gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
11)	gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
12)	gZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
13)	Upon unicast from gZC, DUT ZR2 does not send the APS ACK to gZC via gZR1. 
14)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field shall be set as 1.
15)	 DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
16)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 0.
17)	 gZC waits for apscAckWaitDuration and does not issue fourth transmit counted packet to DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x5B, 0x5C ... ……0x78).
18)	 gZC over the air packet,in the APS header extended header sub-frame of the frame control field is disabled
19)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
20)	 gZC over the air packet, in the APS header block number of the extended header sub-frame does not 03.
21)	 gZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue fifth transmit counted packet to  DUT ZR2 via gZR1 with the payload. (As per the test profile say 0x79, 0x7A, 0x7B……….0x96). 
22)	 gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
23)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
24)	 gZC over the air packet, in the APS header block number of the extended header sub-frame does not 04.
25)	 gZC waits for apsInterframeDelay (say 100 milliseconds) does not issue sixth transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x97, 0x98, 0x99,………. 0xB4).
26)	 gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
27)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
28)	 gZC over the air packet, in the APS header block number of the extended header sub-frame does not 05.
29)	Upon unicast from gZC, DUT ZR2 does not send the APS ACK to gZC via gZR1. 
30)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 1.
31)	 DUT ZR2 over the air packet, in the APS header ACK bit fieldnumber of the extended header sub-frame does not 0xFF.
32)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 03.
33)	 gZC waits for apscAckWaitDuration and does not issue seventh transmit counted packet to  DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xB5, 0xB6, 0xB7……….0xD2).
34)	 gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
35)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
36)	 gZC over the air packet, in the APS header block number of the extended header sub-frame does not 06.
37)	 gZC waits for apsInterframeDelay (say 100 milliseconds) and does not issue eighth transmit counted packet to  DUT ZR2 via gZR1 with the payload. (As per the test profile say 0xD3, 0xD4, 0xD5……0xF0).
38)	 gZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
39)	 gZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10.
40)	 gZC over the air packet, in the APS header block number of the extended header sub-frame does not 07.
41)	Upon unicast from gZC,  DUT ZR2 does not send the APS ACK to gZC via gZR1. 
42)	 DUT ZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 1.
43)	 DUT ZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
44)	 DUT ZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 06.
