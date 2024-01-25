TP/PRO/BV-09 Many-to-one Route repair
Verify the DUT issues many-to-one network error command on failure of many-to-one route discovery.

gZC
gZR1 (00 00 00 01 00 00 00 00)
gZR2 (00 00 00 02 00 00 00 00)
DUT ZR3 (00 00 00 03 00 00 00 00)
gZR4 (00 00 00 04 00 00 00 00)

  gZC
    |
  gZR1 gZR2
    |
  DUT ZR3
    |
  gZR4

Initial conditions:
1. gZR1, gZR2 joins ZC. DUT ZR3 joins PAN with gZR2. gZR4 joins PAN with DUT ZR3.
2. Register end point of 0xf0 and 0x01 in gZC, gZR1, gZR2, DUT ZR3 and gZR4.
3. gZC initiated NLME-ROUTE DISCOVERY.Request
  Dst Addr mode =0x00 –No destination address
  DstAddr=0xFFF9 -To be ignored
  Radius=0x03- (may be assigned to be network broadcast radius)
  NoRouteCache=0x0.

Test procedure:
1. Hard reset gZR1.
2. gZR4 issues the Buffer Test Request to gZC.
  Dst address mode=0x02
  Dst address=0x0000
  Dst end point=0xF0
  Source endpoint=0x01
  Tx option=0x00
  
  Pass verdict:
1) gZR4 shall initiate a Buffer test request to gZC. gZR4 shall unicast the data to DUT ZR3, as updated in the routing table.
2) DUT ZR3 shall route the Buffer test request to gZR1 as updated in the routing table.
3) DUT ZR3 shall retry for three times to route the data (as gZR1 as hard-reset).
4) On failure of this data transmission, DUT ZR3 shall issue many-to-one route error command packet with the code (0x0C) to gZC through gZR2.

  Fail verdict:
1) gZR4 does not initiate a Buffer test request to gZC. gZR4 does not unicast the data to DUT ZR3, as updated in the routing table.
2) DUT ZR3 does not route the Buffer test request to gZR1 as updated in the routing table.
3) DUT ZR3 does not retry for three times to route the data(as gZR1 as hard-reset).
4) On failure of this data transmission, DUT ZR3 does not issue many-to-one route error command packet with the code (0x0C) to gZC through gZR2.
