TP/R21/BV-28 Test for mgmt_bind_req, bind_req, unbind_req
Objective: Demonstrate that the device’s binding table can be modified and queried by a remote device.

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
2 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
  (Criterion 1)
3 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
  (Criterion 2)
4 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #2
  (Criterion 3)
5 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0008, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
  (Criterion 4)
6 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A1, endpoint #1
  (Criterion 5)
7 Let gZR send a ZDO bind_req to DUT for endpoint #1, cluster 0x0006, targeting group 0x1234 (Criterion 6)
8 Let gZR send any number of ZDO mgmt._bind_req commands to DUT required to obtain the complete binding table. (Criterion 7)
9 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
  (Criterion 8)
10 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
   (Criterion 9)
11 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #2
   (Criterion 10)
12 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0008, targeting EUI-64 0x0000 1111 0505 A0A0, endpoint #1
   (Criterion 11)
13 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0006, targeting EUI-64 0x0000 1111 0505 A0A1, endpoint #1
   (Criterion 12)
14 Let gZR send a ZDO unbind_req to DUT for endpoint #1, cluster 0x0006, targeting group 0x1234
   (Criterion 13)
15 Let gZR send any number of ZDO mgmt._bind_req commands to DUT required to obtain the complete binding table.
   (Criterion 14)


Pass Verdict:
1 DUT sends a bind_rsp frame with the status being SUCCESS.
2 DUT sends a bind_rsp frame with the status being SUCCESS.
3 DUT sends a bind_rsp frame with the status being SUCCESS or TABLE_FULL (0x8c).
4 DUT sends a bind_rsp frame with the status being SUCCESS or TABLE_FULL (0x8c).
5 DUT sends a bind_rsp frame with the status being SUCCESS or TABLE_FULL (0x8c).
6 DUT sends a bind_rsp frame with the status being SUCCESS or TABLE_FULL (0x8c).
7 DUT sends a number of mgmt_bind_rsp frames conveying the following bindings:
  1. EP#1, cluster 0x0006 -> 0x0000 1111 0505 A0A0.EP#1
  2. EP#1, cluster 0x0006 -> 0x0000 1111 0505 A0A0.EP#2
  3. EP#1, cluster 0x0008 -> 0x0000 1111 0505 A0A0.EP#1
  4. EP#1, cluster 0x0006 -> 0x0000 1111 0505 A0A1.EP#1
  5. EP#1, cluster 0x0006 -> group 0x1234
  Notice: If the DUT returned TABLE_FULL (0x8c) at any of the above steps, the list of bindings might be shorter
8 DUT sends an unbind_rsp frame with the status being SUCCESS.
9 DUT sends an unbind_rsp frame with the status being NO_ENTRY (0x88).
10 DUT sends an unbind_rsp frame with the status being SUCCESS or NO_ENTRY (0x88) (if the mating bind request did not return SUCCESS).
11 DUT sends an unbind_rsp frame with the status being SUCCESS or NO_ENTRY (0x88) (if the mating bind request did not return SUCCESS).
12 DUT sends an unbind_rsp frame with the status being SUCCESS or NO_ENTRY (0x88) (if the mating bind request did not return SUCCESS).
13 DUT sends an unbind_rsp frame with the status being SUCCESS or NO_ENTRY (0x88) (if the mating bind request did not return SUCCESS).
14 DUT sends an mgmt_bind_rsp frame conveying an empty binding table.


Fail Verdict:
1 DUT does not respond to bind_req or status is not SUCCESS.
2 DUT does not respond to bind_req or status is not SUCCESS.
3 DUT does not respond to bind_req or status is not SUCCESS or TABLE_FULL.
4 DUT does not respond to bind_req or status is not SUCCESS or TABLE_FULL.
5 DUT does not respond to bind_req or status is not SUCCESS or TABLE_FULL.
6 DUT does not respond to bind_req or status is not SUCCESS or TABLE_FULL.
7 DUT does not send mgmt_bind_rsp or number of bindings returns does not match, or duplicate entries exist,
  or the table is truncated although each bind_rsp mentioned above conveyed a status of SUCCESS.
8 DUT does not respond to unbind_req or status is different from SUCCESS.
9 DUT does not respond to unbind_req or status is different from NO_ENTRY.
10 DUT does not respond to unbind_req or status is different from SUCCESS and NO_ENTRY.
11 DUT does not respond to unbind_req or status is different from SUCCESS and NO_ENTRY.
12 DUT does not respond to unbind_req or status is different from SUCCESS and NO_ENTRY.
13 DUT does not respond to unbind_req or status is different from SUCCESS and NO_ENTRY.
14 DUT doess not send mgmt_bind_rsp or number of bindings is non-zero.



Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.
