TP/PRO/BV-40 Data transmission to Router and end device with fragmentation

Verify the successful unicast data transmission to router and an end device with 
fragmentation.  This test case is applicable to both Sub-GHz and 2.4 GHz 
interfaces.

Initial Conditions:
DUT ZC 
PANid= Generated in a random 
manner(within the range 0x1 to 0x3FFF) 
Logical Address = 0x0000 
IEEE address=0xaa aa aa aa aa aa aa aa

gZR1  PANid=same PANID of ZC1. 
Logical Address=Generated in a random 
manner(within the range 1 to 0xFFF7)  
IEEE address=0x00 00 00 01 00 00 00 00 

gZR2  PANid=same PANID  of  ZC1  
Logical Address=Generated in a random 
manner(within the range 1 to 0xFFF7)  
IEEE address=0x00 00 00 02 00 00 00 00 

gZED  PANid=same PANID  of  ZC1  
Logical Address=Generated in a random 
manner(within the range 1 to 0xFFF7)  
IEEE address=0x00 00 00 00 00 00 00 01

Initial Conditions:
1. gZR1 joins DUT ZC.   
2. gZR2, gZED joins gZR1.    
3. Set length of data message to be sent to 150 bytes by some means. 
4. Set apscMaxWindowSize is equal to 1 by some means. Set the fragmentation threshold condition as 50 bytes  
5. Set apsInterframeDelay is equal to 10 milliseconds by some means.

Test Procedure:
1. DUT ZC sends an APSDE-DATA.request to gZR2. 
 
APSDE-DATA.Request 
DstAddrMode = 0x02 (direct address) 
DstAddress = ZR2 address 
DstEndpoint=0xF0 
ProfileId,=0x7f01  
ClusterId=0x0001-Transmit count packet 
SrcEndpoint=0x01 
asduLength=0x96 
asdu=Transmit count with sequence length octet 
TxOptions=0x08 
Radius Counter=0x02

2. DUT ZC sends an APSDE-DATA.request to 
gZED. 
 
APSDE-DATA.Request 
DstAddrMode = 0x02 (direct address) 
DstAddress = DUT ZED address 
DstEndpoint=0xF0 
ProfileId,=0x7f01  
ClusterId=0x0001- Transmit count packet 
SrcEndpoint=0x01 
asduLength=0x96 
asdu=Transmit count with sequence length octet 
TxOptions=0x08 
Radius Counter=0x02

3. gZR2 sends an APSDE-DATA.request to DUT ZC 

APSDE-DATA.Request 
DstAddrMode = 0x02 (direct address) 
DstAddress = ZC address 
DstEndpoint=0xF0 
ProfileId,=0x7f01  
ClusterId=0x0001-Transmit count packet 
SrcEndpoint=0x01 
asduLength=0x96 
asdu=Transmit count with sequence length octet 
TxOptions=0x08 
Radius Counter=0x02 

Pass verdict:
1)	DUT ZC issues first transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 32).
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled. 
3)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b01
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
5)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
6)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
7)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
8)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 00.
9)	DUT ZC waits for apscAckWaitDuration and issues second transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x33, 0x34, 0x35………. 0x64).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
13)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
14)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
15)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
16)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
17)	DUT ZC waits for apscAckWaitDuration and issues third transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x65, 0x66, 0x67……..0x96).
18)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
19)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
20)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
21)	Upon unicast from DUT ZC, gZR2 sends the APS ACK to DUT ZC via gZR1. 
22)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
23)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
24)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
25)	DUT ZC issues first transmit counted packet to gZED via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 32).
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled. 
27)	DUT ZC over the air packet,in the APS header extended frame control field of the extended header sub-frame shall be b01
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 03.
29)	Upon unicast from DUT ZC, gZED sends the APS ACK to DUT ZC via gZR1. 
30)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
31)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
32)	gZED over the air packet, in the APS header block number of the extended header sub-frame shall be 00.
33)	DUT ZC waits for apscAckWaitDuration and issues second transmit counted packetto gZED via gZR1 with the payload. (As per the test profile say 0x33, 0x34, 0x35………. 0x64).
34)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
35)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
36)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
37)	Upon unicast from DUT ZC, gZED sends the APS ACK to DUT ZC via gZR1. 
38)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
39)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
40)	gZED over the air packet, in the APS header block number of the extended header sub-frame shall be 01.
41)	DUT ZC waits for apscAckWaitDuration and issues third transmit counted packetto gZED via gZR1 with the payload. (As per the test profile say 0x65, 0x66, 0x67……………...0x96).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is enabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b10
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
45)	Upon unicast from DUT ZC, gZED sends the APS ACK to DUT ZC via gZR1. 
46)	gZR2 over the air packet, in the APS header the acknowledgement request sub-field of the frame control field shall be set as 0, and the Extended Header Present bit shall be set to 1.  
The Extended Header sub-frame shall be present and the Extended Header frame control shall be set to b01 , indicating fragmentation is present if the ACK is sent in response to the first fragment 
and to b10 if the ACK is sent in response to any subsequent fragment.
47)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame shall be 0xFF.
48)	gZED over the air packet, in the APS header block number of the extended header sub-frame shall be 02.
49)	gZR1 issues first transmit counted packet to DUT with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 32).
50)	In the resulting DUT ACK over the air packet, in the APS header extended header sub-frame of the frame control field is enabled. 
51)	DUT ZC ACK over the air packet, in the APS header extended frame control field of the extended header sub-frame shall be b01

Fail verdict:
1)	DUT ZC does not issue first transmit counted packet to gZR2 via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 32).
2)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled. 
3)	DUT ZC over the air packet,in the APS header extended frame control field of the extended header sub-frame does not b01
4)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 03.
5)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
6)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
7)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
8)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 00.
9)	DUT ZC waits for apscAckWaitDuration and does not issue second transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x33, 0x34, 0x35………. 0x64).
10)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
11)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
12)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
13)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
14)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
15)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
16)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 01.
17)	DUT ZC waits for apscAckWaitDuration and does not issue third transmit counted packetto gZR2 via gZR1 with the payload. (As per the test profile say 0x65, 0x66, 0x67……..0x96).
18)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
19)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
20)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 02.
21)	Upon unicast from DUT ZC, gZR2 does not send the APS ACK to DUT ZC via gZR1. 
22)	gZR2 over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
23)	gZR2 over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
24)	gZR2 over the air packet, in the APS header block number of the extended header sub-frame does not 02.
25)	DUT ZC does not issue first transmit counted packet to DUT ZED via gZR1 with the payload. (As per the test profile say 0x1, 0x2, 0x3……….0x 32).
26)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled. 
27)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b01
28)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 03.
29)	Upon unicast from DUT ZC, gZED does not send the APS ACK to DUT ZC via gZR1. 
30)	gZED over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
31)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
32)	gZED over the air packet, in the APS header block number of the extended header sub-frame does not 00.
33)	DUT ZC waits for apscAckWaitDuration and does not issue second transmit counted packetto gZED via gZR1 with the payload. (As per the test profile say 0x33, 0x34, 0x35………. 0x64).
34)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
35)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
36)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 01.
37)	Upon unicast from DUT ZC, gZED does not send the APS ACK to DUT ZC via gZR1. 
38)	gZED over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
39)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
40)	gZED over the air packet, in the APS header block number of the extended header sub-frame does not 01.
41)	DUT ZC waits for apscAckWaitDuration and does not issue third transmit counted packetto gZED via gZR1 with the payload. (As per the test profile say 0x65, 0x66, 0x67……..0x96).
42)	DUT ZC over the air packet, in the APS header extended header sub-frame of the frame control field is disabled.
43)	DUT ZC over the air packet, in the APS header extended frame control field of the extended header sub-frame does not b10
44)	DUT ZC over the air packet, in the APS header block number of the extended header sub-frame does not 02.
45)	Upon unicast from DUT ZC, gZED does not send the APS ACK to DUT ZC via gZR1. 
46)	gZED over the air packet, in the APS header acknowledgement request sub-field of the frame control field does not set as 0.
47)	gZED over the air packet, in the APS header ACK bit field number of the extended header sub-frame does not 0xFF.
48)	gZED over the air packet, in the APS header block number of the extended header sub-frame does not 02.
49)	DUT ZC does not APS ACK the data received form gZR1
50)	DUT ZC ACK over the air packet, in the APS header extended header sub-frame of the frame control field is disabled. 
51)	DUT ZC ACK over the air packet,in the APS header extended frame control field of the extended header sub-frame is not b01

