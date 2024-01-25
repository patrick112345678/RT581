TP/R21/BV-25 Maximum Frame Buffer Size
Objective: Verify that the DUT ZR/ZC/ZED is capable of receiving and processing a frame of 127 bytes PPDU
           length, i.e. the maxium frame size supported by the PHY/MAC.

Initial Conditions:

DUT - ZR/ZC/ZED

   DUT
    |
    |
   gZR


Test Procedure:
1 Make sure that DUT and gZR (test harness) are on the same network and able to communicate, e.g.
 - if DUT is a ZC, join gZR to DUT ZC;
 - if DUT is a ZR, let gZR form a distributed security network and let DUT ZR join;
 - if DUT is a ZED, let gZR form a distributed security network and let DUR ZED join.
2 Let gZR send a reset packet count message to DUT
3 Let gZR send a retrieve packet count message to DUT
4 Let gZR send 10 transmit counted packets messages to DUT, where the packet length field shall be 81 (0x51),
  resulting in a PPDU length of 127 bytes per frame (the maximum transfer unit supported by the PHY), with APS
  ACK request flag set and MAC ACK request flag set.
  Note: The assumption here is that NWK security is used, APS security is not used. During the test,
  make sure that the ‘transmit counter packets’ frame is 127 bytes long; adjust the packet length if required.
4 Let gZR send a retrieve packet count message to DUT



Pass Verdict:
1 DUT responds with a packet count of 0x0000
2 DUT responds to each transmit counted packets message request with a MAC ACK frame and an APS ACK frame,
  where the respective sequence counters match those of the according ‘transmit counted packets’ test message.
3 DUT responds with a packet count of 0x000A (10).


Fail Verdict:
1 DUT does not respond or responds with a packet count other than 0x0000.
2 DUT does not resond with MAC ACK frame, or sequence number does not match, or DUT does not respond with APS ACK frame or sequence number does not match
3 DUT does not respond or responds with a packet count other than 0x000A.


Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.
