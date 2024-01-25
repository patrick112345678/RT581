4.3.11 CN-NST-TC-10: Network steering with TC_Significance=0x00; DUT: ZC
This test verifies the behavior of the ZC after reception of Mgmt_Permit_Joining_req with TC_Significance=0x00 (as may be sent by legacy devices). This test verifies that the ZC still allows the new device to join, incl. correct network key delivery. 
The device takes the role described in its PICS; the other role can be performed by a test harness or a golden unit, as indicated.
Successful completion of test 4.2.1 is pre-requisite for performing this test for a DUT-ZC.


Required devices:
DUT - ZC of the network; 
ZC uses default global TC-LK for NWK key delivery.
ZC allows for requesting TC-LK update. (AIB allowTrustCenterLinkKeyRequests  attribute set to 0x1).

Additional devices:
THr1 - TH ZR in a centralized network, becoming a parent 
ZR allows for usage of default global TC-LK for joining
THr2 - TH ZR attempting to join a centralized network


Test preparation:
P1a - DUT is powered on. Network formation is triggered on the DUT. DUT successfully forms a centralized network.
P1b - DUT’s TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT, thus network steering has to be triggered locally on the DUT.
P2 - Network steering is triggered on the DUT. THr1 is powered on and network steering is triggered on the THr1. THr1 successfully joins centralized network of the DUT, incl. TC-LK update. Wait for this network steering session to expire.


Test Procedure:
 - Network steering is triggered on the THr1.
   THr1 unicasts to the DUT a Mgmt_Permit_Joining_req, with PermitDuration of at least bdbcMinCommissioningTime (180s), but with: 
   TC_Significance = 0x00.
 - THr2 is placed in radio range of the DUT and powered on.
   Network steering is triggered on the THr2. 
   THr2 sends at least one MAC Beacon Request on the operational channel of the DUT.
 - THr2 chooses DUT as a parent and successfully completes association at the DUT.
