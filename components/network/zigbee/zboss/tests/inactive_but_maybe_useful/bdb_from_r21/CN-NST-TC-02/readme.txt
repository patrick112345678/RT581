4.3.3 CN-NST-TC-02: ZED joining at ZC; DUT: ZC

This test verifies the network steering procedure.
This test verifies the behavior of the THc1 as a parent of a ZED, incl. TC-LK update; successful completion of test 4.2.1 is pre-requisite for performing this test for a DUTc.
The device takes the role described in its PICS; the other role can be performed by a test harness or a golden unit.


Required devices:
DUT - ZC of the network;
ZC uses default global TC-LK for NWK key delivery.
ZC allows for requesting TC-LK update. (AIB allowTrustCenterLinkKeyRequests  attribute set to 0x1).

THe1 - ZED joining the centralized network at the ZC
ZED allows for usage of default global TC-LK for joining


Test preparation:
P1 - DUT is powered on. Network formation is triggered on the DUT. DUT successfully forms centralized network. 
P2 - DUT’s TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT.


Test procedure
Exactly as in 4.3.1, only with THr1 being a THe1.
