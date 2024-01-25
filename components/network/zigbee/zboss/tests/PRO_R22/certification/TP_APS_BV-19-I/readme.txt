11.19 TP/APS/BV-19-I Source Binding – Router
Objective: Verify that DUT Router can store its own binding entries

        GZC
   |            |
DUT ZR1        GZR2

gZC:
PANId=0x01AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1:
Router extended address:
0x 00 00 00 01 00 00 00 00

gZR2:
Router extended address:
0x 00 00 00 02 00 00 00 00

Initial Conditions:
1 DUT ZR1 joins the network at gZC
2 gZR2 joins the network at gZC


Test procedure:
1. DUT ZR1 issues an APSME-BIND.request to bind endpoint 1 to gZR2’s endpoint 240;
2. DUT ZR1 issues an APSDE-DATA.request with address mode 0x00 and with APS ACK enabled;
3. gZR2 issues APS ACK;

Pass verdict:
1) DUT ZR1 issues an APSME-BIND.request that receives Status = Success (or
Not Supported, in which case, the rest of the outcome is N/A) IEEE Address
Request is sent from DUT ZR1 to gZR2.
2) DUT ZR1 sends an APSDE-DATA.request to gZR2 using address mode 0x00.
3) gZR2 sends APS acknowledge.

Fail verdict:
1) DUT ZR1 issues an APSME-BIND.request that receives Status <> Success or
Not_Supported
2) DUT ZR1 does not transmit, or it does not transmit to gZR2.
3) gZR2 does not acknowledge.

 

 
