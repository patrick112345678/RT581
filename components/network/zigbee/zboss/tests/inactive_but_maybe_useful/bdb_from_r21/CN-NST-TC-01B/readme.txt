4.3.2 CN-NST-TC-01B: ZR joining at ZC; DUT: ZC


This test verifies the network steering procedure.
This test verifies the behavior of the ZC as a parent of a ZR, incl. TC-LK update; successful completion of test 4.2.1 is pre-requisite for performing this test for a DUTc.


Required devices:
DUT - ZC of the network;
ZC uses default global TC-LK for NWK key delivery.
ZC allows for requesting TC-LK update. (AIB allowTrustCenterLinkKeyRequests attribute set to 0x1).
THr1 - TH ZR joining the centralized network at the ZC
ZR allows for usage of default global TC-LK for joining


Preparatory step:
P1 - DUT is powered on. Network formation is triggered on the DUT. DUT successfully forms centralized network. 
P2 - DUT’s TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT.


Test procedure:
- Trigger steering on the DUT
- Place THr1 in range of DUT and powered on.
Wait for approx. 120 seconds (to test the Permit Duration).
- Trigger Steering on the THr1
- Wait for THr1 complete join (see test spec)


