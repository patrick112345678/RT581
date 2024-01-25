TP/R21/BV-11 Processing of unencrypted NWK frames
Objective: Objective: Make sure that DUT drops unencrypted NWK frames silently without further processing,
neither locally nor by forwarding/re-broadcasting; The only exceptions are: NWK rejoin request, NWK rejoin
response (to enable TC rejoin after missed key updates), and initial APS transport key.

Initial Conditions:

DUT - ZC/ZR/ZED


DUT, gZR1 and gZR2



Test Pocedure:
1 Make sure that DUT and gZR1 (test harness), gZR2 are on the same network and able to communicate, e.g.
  - if DUT is a ZC, join gZR1, gZR2 to DUT ZC;
  - if DUT is a ZR, let gZR1 form a distributed security network and let DUT ZR and gZR2 join;
  - if DUT is a ZED, let gZR1 form a distributed security network and let DUT ZED and gZR2 join.
2 Let gZR1 send a ZDO node_desc_req to DUT, without NWK securiy
3 Let gZR1 send a ZDO node_desc_req to DUT, this time with NWK security

Note:The steps 4-9 are only executed if DUT is a ZR/ZC, not a ZED

4 Create a routing table entry on gZR1 with destination address = gZR2, next hop = DUT ZR/ZC.
5 Let gZR1 send a ZDO node_desc_req to gZR2, without NWK security
6 Let gZR1 send a ZDO node_desc_req to gZR2, this time with NWK security
7 Turn off gZR2. This shall have the effect of disabling any passive acknowledgment scheme in the DUT.
  Perform the next steps within 15 to 30 seconds (such that the neighbour table on DUT still has a valid entry for gZR2)
8 Let gZR1 broadcast a ZDO nwk_addr_req for IEEE 0x0001000000000000, destination address 0xfffd, without NWK security
9 Let gZR1 broadcast a ZDO nwk_addr_req for IEEE 0x0001000000000000, destination address 0xfffd, with NWK security

Note: DUT shall further pass test 10.62 or 10.64 to demonstrate its ability to process rejoin requests and responses
      not encrypted at the NWK layer (TC rejoin), if that test was not run earlier.


Pass verdict:
1 DUT does not respond to node_desc_req.
2 DUT sends a node_desc_rsp frame.
3 DUT does not forward the node_desc_req to gZR2.
4 DUT forwards the node_desc_req frame to gZR2.
5 DUT does not rebroadcast the nwk_addr_req.
6 DUT rebroadcasts the nwk_addr_req.


Fail verdict:
1. DUT sends a node_desc_rsp frame.
2. DUT does not respond to node_desc_req,
3. DUT forwards the node_desc_req frame to gZR2.
4. DUT does not forward the node_desc_req frame to gZR2,
5. DUT rebroadcasts the nwk_addr_req.
6. DUT does not rebroadcast the nwk_addr_req,


Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.
 - If DUT is ZED or ZR gZR1 will wait 30 seconds before test start to ensure that DUT and gZR2 (if DUT is ZR) has been exchanged Link Status commands and add each other in neighbor table (if not alerady present). Such delay for DUT ZED caused because gZR1 sorce code is same for DUT ZR and DUT ZED test cases.
