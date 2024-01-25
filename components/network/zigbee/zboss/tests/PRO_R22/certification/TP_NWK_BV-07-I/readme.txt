11.7 TP/NWK/BV-07-I Network Broadcast to Router
Objective:  Broadcast to Router only

Initial Conditions:
     gZC
   DUT_ZR1
gZED1    gZED2

gZC:
PANid= 0x1AAA
Logical Address = 0x0000
0x aa aa aa aa aa aa aa aa

DUT_ZR1:
PANid= 0x1AAA
0x 00 00 00 01 00 00 00 00

gZED1:
PANid= 0x1AAA
0x 00 00 00 09 00 00 00 01

gZED2:
PANid= 0x1AAA
0x 00 00 00 00 00 00 00 02


1 Reset nodes
2 Set gZC under target stack profile, gZC as coordinator starts a PAN = 0x1AAA network
3 DUT ZR1 joins gZC
4 gZED1 has macRxOnWhenIdle=TRUE; it joins at DUT ZR1
5 gZED2 joins at DUT ZR1; macRxOnWhenIdle=FALSE
where gZC, gZED1 and gZED2 are golden units.

Test procedure:
1. gZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xffff
2. gZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xfffc
3. gZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xfffd

Test Set-Up:
The gZC shall be in wireless communication proximity of DUT ZR1; DUT ZR1 and gZED1/gZED2 shall be in communication range.
A packet sniffer shall be observing the communication over the air interface. 

Pass verdict:
1) Upon broadcast (0xffff) from gZC, the Test Buffer Request is received by DUT ZR1 and a Test Buffer Response is sent from DUT ZR1 back to gZC.
2) The Test Buffer Request (NWKaddr=0xffff) is forwarded by DUT ZR1 via unicast to gZED1 and gZED2. Test Buffer Response is sent from gZED1 and gZED2 via DUT ZR1 back to gZC.
3) Upon broadcast (0xfffc) from gZC, the Test Buffer Request is received by DUT ZR1 and a Test Buffer Response is sent from DUT ZR1 back to gZC.
4) Upon broadcast (0xfffd) from gZC, the Test Buffer Request is received by DUT ZR1 and a Test Buffer Response is sent from DUT ZR1 back to gZC.
5) Upon broadcast (0xfffd) from gZC, the Test Buffer Request is received by gZED1 and a Test Buffer Response is sent from gZED1 back to gZC.

Fail verdict:
1) The Test Buffer Request (NWKaddr=0xffff) is not forwarded by DUT ZR1 via unicast to gZED1 and gZED2. Test Buffer Response is not sent from gZED1 and gZED2 via gZR1 back to gZC.
2) Upon broadcast (0xfffc) from gZC, the Test Buffer Request is received by DUT ZR1 and a Test Buffer Response is not sent from DUT ZR1 back to gZC.
3) Upon broadcast (0xfffd) from gZC, the Test Buffer Request is received by DUT ZR1 and a Test Buffer Response is not sent from DUT ZR1 back to gZC.
4) Upon broadcast (0xfffd) from gZC, the Test Buffer Request is received by gZED1 and a Test Buffer Response is not sent from gZED1 back to gZC.