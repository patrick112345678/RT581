11.6 TP/NWK/BV-06-I Network Broadcast to Coordinator
Objective: Broadcast from Coordinator, going through three scenarios of NWK broadcast (3.7.5)

Initial Conditions:
    DUT ZC
     gZR1
gZED1    gZED2

gZED1 has macRxOnWhenIdle=TRUE; it joins at gZR1
gZED2 joins at gZR1; macRxOnWhenIdle=FALSE

ZC DUT:
PANid= 0x1AAA
Logical Address = 0x0000
0x aa aa aa aa aa aa aa aa

gZR1:
PANid= 0x1AAA
0x 00 00 00 01 00 00 00 00

gZED1:
PANid= 0x1AAA
0x 00 00 00 09 00 00 00 01

gZED2:
PANid= 0x1AAA
0x 00 00 00 00 00 00 00 02

1 Reset nodes
2 Set ZC under target stack profile, ZC as coordinator starts a PAN = 0x1AAA network
3 gZR1 joins DUT ZC
4 gZED1 has macRxOnWhenIdle=TRUE; it joins at gZR1
5 gZED2 joins at gZR1; macRxOnWhenIdle=FALSE
where gZR1, gZED1 and gZED2 are golden units.


Test procedure:
1. DUT ZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xffff
2. DUT ZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xfffc
3. DUT ZC issues a broadcast Test Buffer Request (0x001c) with NWK address set at 0xfffd


Test Set-Up:
The DUT ZC shall be in wireless communication proximity of gZR1; gZR1 and gZED1/gZED2 shall be in communication range.
A packet sniffer shall be observing the communication over the air interface.

Pass verdict:
1) Upon broadcast (0xffff) from DUT ZC, the Test Buffer Request is received by gZR1 and a Test Buffer Response is sent from gZR1 back to DUT ZC.
2) The Test Buffer Request (NWKaddr=0xffff) is forwarded by gZR1 via unicast to gZED1 and gZED2. 
Test Buffer Response is sent from gZED1 and gZED2 via gZR1 back to DUT ZC.
3) Upon broadcast (0xfffc) from DUT ZC, the Test Buffer Request is received by gZR1 and a Test Buffer Response is sent from gZR1 back to DUT ZC.
4) Upon broadcast (0xfffd) from DUT ZC, the Test Buffer Request is received by gZR1 and a Test Buffer Response is sent from gZR1 back to DUT ZC.
5) Upon broadcast (0xfffd) from DUT ZC, the Test Buffer Request is received by gZED1 and a Test Buffer Response is sent from gZED1 back to DUT ZC.

Fail verdict:
1) The Test Buffer Request (NWKaddr=0xffff) is not forwarded by gZR1 via unicast to gZED1 and gZED2. 
Test Buffer Response is not sent from gZED1 and gZED2 via gZR1 back to DUT ZC.
2) Upon broadcast (0xfffc) from DUT ZC, the Test Buffer Request is received by gZR1 and a Test Buffer Response is not sent from gZR1 back to DUT ZC.
3) Upon broadcast (0xfffd) from DUT ZC, the Test Buffer Request is received by gZR1 and a Test Buffer Response is not sent from gZR1 back to DUT ZC.
4) Upon broadcast (0xfffd) from DUT ZC, the Test Buffer Request is received by gZED1 and a Test Buffer Response is not sent from gZED1 back to DUT ZC.





