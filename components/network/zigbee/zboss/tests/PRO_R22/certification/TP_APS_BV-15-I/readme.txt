11.15 TP/APS/BV-15-I Group Management-Group Addition – Tx
Objective: Verify that DUT can transmit a group-destined broacast to an implicit endpoint.

    DUT_ZC
  |        |
  |        |
 gZR1     gZR2

DUT ZC:
PANId=0x01AAA
0x0000
Sender external address:
0x aa aa aa aa aa aa aa aa 

gZR1:
Assigned 0x0001
Receiver extended address:
0x 00 00 00 01 00 00 00 00

gZR2:
Assigned 0x0002
Extended address:
0x 00 00 00 01 00 00 00 00


Group Address 0x0001: gZR1, gZR2 members with endpoint 0xF0.

Initial conditions:
1. gZR1 and gZR2 join the network at DUT ZC
2. Assign group address 0x0001 to gZR1, gZR2 group members, with endpoint assignment of 0xf0, by implementation-specific means.

Test procedure:
1. DUT ZC shall transmit to gZR1 a Test Buffer Request (cluster Id=0x001c) to endpoint 0xF0;
2. DUT ZC shall NWK broadcast (NWK address = 0xfff) a Test Buffer Request (cluster Id=0x001c) to group address 0x0001;

Pass verdict:
1) DUT ZC unicasts a Test Buffer Req to gZR1 (in the Test Specification there is a mistake - "broadcasts").
2) gZR1 responds with Test Buffer Rsp to DUT ZC from endpoint 0xF0.
3) DUT ZC broadcasts a Test Buffer Req to Group 0x0001 with no destination endpoint.
4) gZR2 and gZR1 respond with Test Buffer Rsp to DUT ZC from endpoint 0xF0.

Fail verdict:
1) DUT ZC sends a Test Buffer Req to gZR1.gZR1 does not respond with Test Buffer Rsp to DUT ZC from endpoint 0xF0.
2) DUT ZC broadcasts a Test Buffer Req to Group 0x0001. gZR2 and gZR1 does not respond with Test Buffer Rsp to DUT ZC from endpoint 0xF0.











