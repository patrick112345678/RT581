TP/PRO/BV-06 Many-to-one Routing (High RAM concentrator)
Verify the DUT is able to initiate a many-to-one route discovery request with high concentrator RAM.Initial Conditions.

DUT ZC
gZR1 (00 00 00 01 00 00 00 00)
gZR2 (00 00 00 02 00 00 00 00)
gZR3 (00 00 00 03 00 00 00 00)

  DUT ZC
    |
  gZR1
    |
  gZR2
    |
  gZR3

Initial conditions:
1. gZR1 joins DUT ZC. gZR2 joins PAN with gZR1. gZR3 joins PAN with gZR2.
2. Register end point of 0xf0 and 0x01 in DUT ZC, gZR1, gZR2 and gZR3.

Test procedure:
1. DUT ZC initiated NLME-ROUTE DISCOVERY.Request
  Dst Addr mode =0x00 –No destination address
  DstAddr=0xFFFC
  Radius=0x03- (may be assigned to be network broadcast radius)
  NoRouteCache=0x0.
2. Instigate buffer test request on gZR3 destined at gZC
  
  Pass verdict:
1) DUT ZC broadcasts Route Discovery request.
  Dst Addr mode =0x00 -No destination address
  DstAddr=0xFFFC
  Radius=0x03 (may be assigned to be network  broadcast radius)
  NoRouteCache=0x0(not memory constrained)
2) DUT ZC over the air packet, frame type sub-field shall be set to 0b01((NWK command) in frame control byte of NWK layer.
3) Upon broadcast route request (0xfffc) from DUT ZC, gZR1 rebroadcast the frame with network radius decremented by 1(0x01).
4) Upon broadcast route request command (0xfffc) from gZR2, gZR2 rebroadcast the frame with network radius decremented by 1(0x00).
5) Upon broadcast (0xFFFC) from gZR2, gZR3 does not rebroadcast.
6) gZR3 constructs route-record frame and transmits it ahead of buffer test request along the path gZR3 -> gZR2 -> gZR1 -> DUT ZC. DUT ZC is
now aware of the route to gZR3. Buffer test request follows the same path.
7) DUT ZC constructs buffer test respose using the source routing information obtained in step 6 and uses source routing, i.e. the NWK
header observed over-the-air contains the source route gZR1 -> gZR2 -> gZR3 in front of the buffer test response payload

  Fail verdict:
1) DUT ZC does not broadcast Route Discovery request.
  Dst Addr mode =0x00 -No destination address
  DstAddr=0xFFFC
  Radius=0x03 (may be assigned to be network  broadcast radius)
  NoRouteCache=0x0(not memory constrained)
2) DUT ZC over the air packet, frame type sub-field does not set to 0b01 ((NWK command) in frame control byte of NWK layer.
3) Upon broadcast route request (0xfffC) from DUT ZC, gZR1 does not rebroadcast the frame with network radius decremented by 1(0x01).
4) Upon broadcast route request command (0xfffC) from gZR2, gZR2 does not rebroadcast the frame with network radius decremented by 1(0x00).
5) Upon broadcast (0xFFFC) from gZR2, gZR3 will rebroadcast.
6) DUT ZC does not use the provided source route or DUT ZC initiates route request for gZR3





