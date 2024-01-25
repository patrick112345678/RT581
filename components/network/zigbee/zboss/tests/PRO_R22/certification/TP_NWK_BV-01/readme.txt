11.1 TP/NWK/BV-01 PAN same-channel coexistence
Objective: Verify that DUT can a PAN in same channel with another PAN

DUT network:
DUT ZC
DUT ZR1
DUT ZED1

Golden unit network:
gZC
gZR1
gZED1

DUT ZC, DUT ZR1 and DUT ZED1 are in communication distance;

DUT ZC: 	PANId=0x1AAA; 0x0000; Coordinator external address: 0x aa aa aa aa aa aa aa aa;
DUT ZR1:	PANId=0x1AAA; 0x 00 00 00 01 00 00 00 00;
DUT ZED1: 	PANId=0x1AAA; 0x 00 00 00 00 00 00 00 01;
gZC:		PANId=0x1AAB; 0x0000; 0x bb bb bb bb bb bb bb bb;
gZR1:		PANId=0x1AAB; 0x 00 00 00 02 00 00 00 00;
gZED1:		PANId=0x1AAB; 0x 00 00 00 00 00 00 00 02;

Initial Conditions:
1. Set up the golden unit network, and generate packet traffic using ClusterId=0x0001, at two second interval,
   length 60 bytes, from gZED1 to gZR1.

Test procedure:
1. DUT ZC starts a PAN;
2. DUT ZED1 joins DUT ZC;
3. DUT ZR1 joins DUT ZC;
4. DUT ZED1 sends packets using ClusterId=0x0001, at two second interval, length 60 bytes, from DUT ZED1 to ZR1;

Pass verdict:
1) DUT ZC is able to start a PAN in presence of a reference network.
2) DUT ZED1 is able to join DUT ZC
3) DUT ZR1 is able to join DUT ZC.
4) DUT ZED1 is able to send packets to DUT ZR1 through DUT ZC.

Fail verdict:
1) DUT ZC is not able to start a PAN in presence of a reference network.
2) DUT ZED1 is not able to join DUT ZC
3) DUT ZR1 is not able to join DUT ZC.
4) DUT ZED1 is not able to send packets to DUT ZR1 through DUT ZC.
