4.3.8 CN-NST-TC-07: Network steering triggered on non-factory-new ZED; DUT: ZED

This test verifies the network steering procedure. 
It verifies the capability of a ZED already being part of the network to trigger network steering on other devices.
The DUT has to take the role as specified in its PICS; the other role can be performed by a test harness or by golden units.


Required devices
THc1 - TH ZC of the network;
ZC uses default global TC-LK for NWK key delivery.
ZC allows for requesting TC-LK update. (AIB allowTrustCenterLinkKeyRequests  attribute set to 0x1).

DUT - ZED joined to the centralized network at the THc1; 


Test preparation:
Preparatory steps:
P1a - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network. Network steering is triggered on THc1.
P1b - DUT is powered on. Network steering is triggered on the DUT.
P2 - DUT successfully joins the centralized network at THc1, incl. successful TC-LK update and verification.
P3 - Network is closed (AssociationPermit of THc1 is set to FALSE, e.g. because it expires).
P4 - THc1 is operational on a centralized network and sends Link Status messages. 
DUT is operational on network of THc1.
P5 - THc1’s TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the THc1.


Test Procedure:
 - Trigger network steering on the DUT.

