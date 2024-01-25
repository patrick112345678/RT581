11.18 TP/APS/BV-18-I Group Management-Group Remove All
Objective: Test DUT's group table size.

gZC:
PANId=0x1AAA
Logical Address=0x0000
0xaa aa aa aa aa aa aa aa

DUT ZR1:
PANid=0x1AAA
Logical Address=0x0001
0x00 00 00 01 00 00 00 00

Initial conditions:
1 Reset gZC
2 Set gZC under target stack profile, as coordinator starts a PAN = 0x1AAA network
3 Join DUT ZR1 to gZC.
4 By implementation specific means, join Group=0x0001 and Group=0x0002 for endpoint 0xf0 on DUT ZR1.

Test procedure:
1. gZC sends Test Buffer request to Group 0x0001 and 0x0002
2. By implementation specific means, REMOVE-ALL-GROUPS on DUT ZED1
3. gZC sends Test Buffer request to Group 0x0001 and 0x0002

1) gZC sends Test Buffer Req to DUT ZR1 from endpoint 0x01 to endpoint 0xf0 for both transmissions on Group 0x0001 and 0x0002.
2) DUT ZR1 sends Test Buffer Rsp to gZC from endpoint 0xf0 to endpoint 0x01 for both transmissions on Group 0x0001 and 0x0002 of gZC.
3) DUT ZR1 does not respond with Test Buffer Rsp to gZCs Test Buffer Req from endpoint 0x01 to endpoint 0xf0 after removal of all groups.

Fail verdict:
1) gZC sends Test Buffer Req to DUT ZR1 from endpoint 0x01 to endpoint 0xf0 for both transmissions on Group 0x0001 and 0x0002. DUT ZR1 does not send Test Buffer Rsp to gZC from endpoint 0xf0 to endpoint 0x01 for both transmissions on Group 0x0001 and 0x0002 of gZC.
2) DUT ZR1 sends response with Test Buffer Rsp to gZCs Test Buffer Req from endpoint 0x01 to endpoint 0xf0 after removal of all groups.

