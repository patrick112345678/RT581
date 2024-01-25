TP/PRO/BV-10 Many-to-one Routing (Low RAM Concentrator)
Verify the DUT is able to initiate route record for a memory constrained concentrator for each message. This test case is applicable to both Sub-GHz and 2.4 GHz interfaces.

gZC
gZR1 (00 00 00 01 00 00 00 00)
DUT ZR2 (00 00 00 02 00 00 00 00)

  gZC
    |
  gZR1
    |
  DUT ZR2

Initial conditions:
1. gZR1 joins gZC. DUT ZR2 joins PAN with DUT ZR1.
2. gZC Initiated NLME-ROUTE DISCOVERY.Request
  Dst Addr mode =0x00 –No destination address
  DstAddr=0xFFFC
  Radius=0x02- (may be assigned to be network broadcast radius)
  NoRouteCache=0x1

Test procedure:
1. DUT ZR2 issues the Buffer Test Request to gZC.
  Dst address mode=0x02
  Dst address=0x0000
  Dst end point=0xF0
  Source endpoint=0x01
  Tx option=0x00
2. DUT ZR2 issues the Buffer Test Request to gZC. 
  Dst address mode=0x02
  Dst address=0x0000
  Dst end point=0xF0
  Source endpoint=0x01
  Tx option=0x00
  
  Pass verdict:
1) DUT ZR2 shall issues the route record before sending the transmitted count packet.
2) DUT ZR2 over the air packet, the relay count field of route record shall set as 0.
3) DUT ZR2 over the air packet, the source address of the route record shall be DUT ZR2 address.
4) The destination address of the route record shall be gZC address.
5) gZR1 shall increment the relay count field record by 1 and its short address appended in the relay list.
6) gZR1 shall unicast the route record frame to gZC.
7) DUT ZR2 shall send the Buffer Test Request to gZC through gZR1.
8) gZC shall receive the Buffer Test Request.
9) DUT ZR2 shall issues the route record before sending the Buffer Test Request.
10) DUT ZR2 over the air packet, the relay count field of route record shall set as 0.
11) DUT ZR2 over the air packet, the source address of the route record shall be DUT ZR2 address.
12) The destination address of the route record shall be gZC address.
13) gZR1 shall increment the relay count field record by 1 and its short address appended in the relay list.
14) gZR1 shall unicast the route record frame to gZC.
15) DUT ZR2 shall send the Buffer Test Request to gZC through gZR1.
16) gZC shall receive the Buffer Test Request.

  Fail verdict:
1) DUT ZR2 does not shall issues the route record before sending the transmitted count packet.
2) DUT ZR2 over the air packet, the relay count field of route record does not set as 0.
3) DUT ZR2 over the air packet, the source address of the route record does not DUT ZR2 address.
4) The destination address of the route record does not gZC address.
5) gZR1 does not increment the relay count field record by 1 or its short address is not appended the relay list.
6) gZR1 does not unicast the route record frame to gZC.
7) DUT ZR2 does not send the Buffer Test Request to gZC through gZR1.
8) gZC does not receive the Buffer Test Request.
9) DUT ZR2 does not shall issues the route record before sending the transmitted count packet.
10) DUT ZR2 over the air packet, the relay count field of route record does not set as 0.
11) DUT ZR2 over the air packet, the source address of the route record does not contain DUT ZR2 address.
12) The destination address of the route record does not gZC address.
13) gZR1 does not increment the relay count field record by 1 or its short address is not appended the relay list.
14) gZR1 does not unicast the route record frame to gZC.
15) DUT ZR2 does not send the Buffer Test Request to gZC through gZR1.
16) gZC does not receive the Buffer Test Request.





