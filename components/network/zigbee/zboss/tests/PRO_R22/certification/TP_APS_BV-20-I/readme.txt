11.20 TP/APS/BV-20-I Source Binding – Table Full

Objective: Verify that DUT Router can store its own binding entries and that it detects and
properly reports when the binding table becomes full. The test loops until it detects
completion (or failure) because there is no mechanism to provide the binding table’s size to
another node. Note: equivalent tests say something like “Use implementation specific
means to fill the source binding table”. This may be preferable.

GZC
|
DUT ZR1

gZC:
PANId=0x01AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1:
Router extended address:
0x 00 00 00 01 00 00 00 00

Initial Conditions:
1 DUT ZR1 joins the network at gZC

1. gZC issues a Bind_Req to DUT ZR1 to bind DUT ZR1 endpoint 1 to gZC endpoint 240;
2. DUT ZR1 issues Bind_rsp;
3. gZC issues Bind_req to DUT ZR1 with source endpoint increased by one;
4. DUT ZR1 issues Bind_rsp. If Bind_rsp status = Success and binding source endpoint < 240 go to 3;
5. Case 1: DUT ZR1’s Bind_rsp status = TABLE_FULL;
6. Case 2: DUT ZR1’s Bind_rsp status <> TABLE_FULL;

Pass verdict:
1) DUT ZR1 issues a Bind_rsp with Status = Success (or Not Supported, in which case, the rest of the outcome is N/A);
2) DUT ZR1 issues a Bind_rsp with Status = Success or Table Full;
3) DUT ZR1 issues a Bind_rsp with Status = Table Full;

Fail verdict:
1) DUT ZR1 issues an APSME-BIND.request that receives Status <> Success or Not_Supported;
2) DUT ZR1 issues a Bind_rsp with Status <> Success or Table Full or source endpoint reaches 241;
3) DUT ZR1 issues a Bind_rsp with Status <> Table Full;




