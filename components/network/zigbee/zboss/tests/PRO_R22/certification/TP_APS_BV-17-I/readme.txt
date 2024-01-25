11.17 TP/APS/BV-17-I Group Management-Group Remove
Objective: Test DUT's group table size.

   gZC
    |
    |
 DUT_ZR1

gZC:
PANId=0x01AAA
0x0000
Sender external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1:
Assigned 0x0001
Receiver extended address:
0x 00 00 00 01 00 00 00 00

Initial conditions:
1. DUT ZED1 joins the network at gZC
2. Assign group address 0x0001, 0x0002 to DUT ZED1 group members, with endpoint
 assignment of 0xf0 by implementation-specific means

Test procedure:
1. gZC by implementation specific means, enter group address 0x0001 and correlate it to endpoint 0xF0.
2. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to group address 0x0001
3. gZC by implementation specific means, enter group address 0x0002 and correlate it to endpoint 0xF0.
4. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to group address 0x0002
5. DUT ZR1 by implementation specific means, delete group address 0x0002 items from its group table
6. gZC NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to group address 0x0002


Pass verdict:
1) DUT ZED1 responds with Test Buffer Rsp to gZC from endpoint 0xF0.
2) DUT ZED1 responds with Test Buffer Rsp to gZC from endpoint 0xF0.
3) DUT ZED1 doen't respond gZC's Test Buffer Rsp

Fail verdict:
1) gZC broadcasts a Test Buffer Req to Group 0x0001 and 0x0002 respectively. DUT ZED1 does not respond with 
   Test Buffer Rsp to gZC from endpoint 0xF0.
2) After DUT ZR1 removed group address 0x002 from its group table, gZC broadcasts a Test Buffer Req to 
   Group 0x0002, and DUT ZED1 responds with Test Buffer Rsp to gZC from endpoint 0xF0.
