TP/PRO/BV-11 Source Route Repair
Force router with conflicting address to resolve its short address

Test procedure for HW (2 ARM/UZ, 2 GP, DURT is ZR2)
- start zc, wait for beacon req - ARM
./zc 1 1
- start zr 2, wait for join complete - GP new
./zr 1
- start zr 3, wait for join complete - GP old
./zr 2
- start zr 4, wait for join complete, then for source record packet - ARM
./zr 3
- switch off zr3 (just interrupt test is not enough)
- 60s after start zc sends data to r4
- verify that after some retries zr2 issues Network status with code 0x0b.



gZC
DUT ZR2 (0x00 00 00 02 00 00 00 00)
DUT ZR3 (0x00 00 00 03 00 00 00 00)
DUT ZR4 (0x00 00 00 04 00 00 00 00)

  gZC
    |
  DUT ZR2
    |
  DUT ZR3
    |
  DUT ZR4

Initial conditions:
1. DUT ZR2 joins PAN withgZC .gZR3 joins PAN with DUT ZR2. gZR4 joins PAN with DUT ZR3.
2. Register end point of 0xf0 and 0x01 in gZC, DUT ZR2, gZR3 and gZR4.
3. gZC initiated NLME-ROUTE DISCOVERY.Request
  Dst Addr mode =0x00 –No destination address
  DstAddr=0xFFF9- To be ignored
  Radius=0x02(may be assigned to be network broadcast radius)
  NoRouteCache=0x0
4. The source record entry is created in gZC.

Test procedure:
1. Hard reset gZR3
2. gZC issues the Transmit count packet to gZR4.
  Packet length=0x0A
  Dst address mode=0x02
  Dst address=gZR4 address
  Dst end point=0xF0
  Source endpoint=0x01
  No of packets=0x1
  Interval=1sec
  Tx option=0x00
  
Pass verdict:
1) gZC shall initiate a transmit count packet to gZR4. gZC shall unicast the data to DUT ZR2, as updated in the source routing table.
2) gZC over the air packet, the relay count subfield of the source route subframe shall have the value of two.
3) gZC over the air packet, the relay index subfield of the source route subframe shall have the value of one.
4) gZC over the air packet, the relay list subfield of the source route subframe shall have the address in the order of g ZR3, DUT ZR2
5) gZC over the air packet, the network destination address shall have the address of gZR4.
6) DUT ZR2 over the air packet, the relay count subfield of the source route subframe shall have the v  alue of two.
7) DUT ZR2 over the air packet, the relay index subfield of the source route subframe shall have the value of zero.
8) DUT ZR2 shall retry for three times to route the data (as gZR3 as hard- reset)
9) On failure of this data transmission, DUT ZR2 shall issue Network Status command packet with the code (0x0B-Source route failure) to gZC concentrator.

Fail verdict:
1) gZC does not initiate a transmit count packet to gZR4. gZC does not l unicast the data to DUT ZR2, as updated in the source routing table.
2) gZC over the air packet, the relay count subfield of the source route subframe does not have the value of three.
3) gZC over the air packet, the relay index subfield of the source route subframe does not have the value of two.
4) gZC over the air packet, the relay list subfield of the source route subframe does not have the address in the order of DUT ZR3,DUT ZR2
5) gZC over the air packet, the network destination address does not have the address of gZR4.
6) DUT ZR2 over the air packet, the relay count subfield of the source route subframe does not have the value of three.
7) DUT ZR2 over the air packet, the relay index subfield of the source route subframe does not have the value of one.
8) DUT ZR2 over the air packet, the relay index subfield of the source route subframe does not have the value of zero and unicast the message to DUTZR3.
9) DUT ZR2 does not retry for three times to route the data(as gZR3 as hard- reset)
10) On failure of this data transmission, DUT ZR2 does not issue Network Status command packet with the code (0x0B-Source route failure) to gZC concentrator.








