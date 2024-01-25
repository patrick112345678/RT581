TP/PRO/BV-08 Route record Update
Verify the route record command has been issued to record the route in its payload, before a unicast packet delivered to the destination.

gZC
DUT ZR1 (00 00 00 01 00 00 00 00)
DUT ZR2 (00 00 00 02 00 00 00 00)
DUT ZR3 (00 00 00 03 00 00 00 00)

  gZC
    |
  DUT ZR1
    |
  DUT ZR2
    |
  DUT ZR3

Initial conditions:
1. DUT ZR1 joins gZC. DUT ZR2 joins PAN with DUT ZR1. DUT ZR3 joins PAN with DUT ZR2.
2. Register end point of 0xf0 and 0x01 in gZC, DUT ZR1, DUT ZR2 and DUT ZR3.
3. gZC initiated NLME-ROUTE DISCOVERY.Request
  Dst Addr mode =0x00 –No destination address
  DstAddr=0xFFFC
  Radius=0x03- (may be assigned to be network broadcast radius)
  NoRouteCache=0x0.

Test procedure:
1. DUT ZR3 issues the Buffer Test Request to gZC.
  Dst address mode=0x02
  Dst address=0x0000
  Dst end point=0xF0
  Source endpoint=0x01
  Tx option=0x00
2. DUT ZR3 issues the Buffer test request to gZC.
  Dst address mode=0x02
  Dst address=0x0000
  Dst end point=0xF0
  Source endpoint=0x01
  Tx option=0x00
  
  Pass verdict:
1) DUT ZR3 shall issues the route record before sending the Buffer test request
2) DUT ZR3 over the air packet, the relay count field of route record shall set as 0.
3) DUT gZR3 over the air packet, the source address of the route record shall be gZR3 address.
4) The destination address of the route record shall be gZC address.
5) DUT ZR2 shall increment the relay count field record by 1 and its short address appends the relay list.
6) DUT ZR1 shall append its network address in the relay list and increase the relay count.
7) DUT ZR1 shall unicast the route record frame to gZC.
8) DUT ZR3 shall send the Buffer test request to gZC through DUT ZR2 and gZR1.
9) gZC shall receive the Buffer test request and respond with a Buffer test response using source routing
10) DUT ZR3 does not issue the route record before sending the Buffer test request
11) DUT ZR3 shall send the Buffer test request to gZC through DUT ZR2 and gZR1.
12) gZC shall receive the B uffer test request and respond with a Buffer test response using source routing

  Fail verdict:
1) DUT ZR3 does not issues the route record before sending the Buffer test request
2) DUT ZR3 over the air packet, the relay count field of route record does not set as 0.
3) DUT ZR3 over the air packet, the source address of the route record does not DUT ZR3 address.
4) The destination address of the route record does not gZC address.
5) DUT ZR2 does not increment the relay count field record by 1 and its short address appends the relay list.
6) DUT ZR1 does not append it network address appends in relay list and increase the relay count.
7) DUT ZR1 does not unicast the route record frame to gZC.
8) DUT ZR3 does not send the Buffer test request to gZC through DUT ZR2 and DUT ZR1.
9) gZC does not receive the Buffer test request
10) DUT ZR3 has issued the route record before sending the Buffer test request
11) DUT ZR3 does not send the buffer test request to gZC through DUT ZR2 and DUT ZR1.
12) gZC does not receive the Buffer test request




