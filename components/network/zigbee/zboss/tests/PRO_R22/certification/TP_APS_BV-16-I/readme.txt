11.16 TP/APS/BV-16-I Group Management-Group Addition (FULL)
Objective: Test DUT's group table size.

  gZC
   |
   |
 DUT_ZR

gZC:
PANId=0x01AAA
0x0000
Sender external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1
Assigned 0x0001
Receiver extended address:
0x 00 00 00 01 00 00 00 00

Initial conditions:
1. DUT ZR1 joins the network at gZC
2. Assign group address 0x0001, 0x0002,0x0003.... to DUT ZR1 group members, with endpoint
assignment of 0xf0 by implementation-specific means till the group table is full

Test procedure:
1. gZC NWK broadcast (NWK address = 0xffff) a Test Buffer Request (cluster Id=0x001c) to endpoint 0xF0.
2. gZC by implementation specific means, enter group address 0x0001 and correlate it to endpoint 0xF0.
3. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to Group 0x0001.
4. gZC by implementation specific means, enter DUT ZR1 maxinum support group address and correlate it to endpoint 0xF0.
5. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to maximum support group address.
6. gZC by implementation specific means, enter DUT ZR maxinum support group address+1 and correlate it to endpoint 0xF0.
7. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to maximum support group address +1.

Pass verdict:
1) gZC broadcasts a Test Buffer Req to DUT ZR1
2) DUT ZR1 responds with Test Buffer Rsp to gZC from endpoint 0xF0.
3) DUT ZR1 responds with Test Buffer Rsp to gZC from endpoint 0xF0
4) DUT ZR1 doen't respond gZC's Test Buffer Rsp           

Fail verdict:
1) gZC broadcasts a Test Buffer Req to DUT ZR1. DUT ZR1 doen't respond with Test Buffer Rsp to DUT ZC from endpoint 0xF0.
2) gZC broadcasts a Test Buffer Req to Group 0x0001 and maximum support group address respectively. 
DUT ZR1 does not respond with Test Buffer Rsp to gZC from endpoint 0xF0.
3) gZC broadcasts a Test Buffer Req to maximum support group address +1, and DUT ZR1 still responds with Test Buffer Rsp to gZC from endpoint 0xF0.
















