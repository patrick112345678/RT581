4.3.6 CN-NST-TC-05: joining via ZR; DUT: parent ZR

This test verifies the network steering procedure of ZR joining one hop away from the ZC. The parent ZR is the DUT. 
This test verifies parent behavior during multi-hop joining of a centralized network, incl. NWK key delivery and TCLK update.


Required devices:
DUT - ZR joined to a centralized network at the ZC, and then becoming a parent itself

THc1 - TH ZC of the network; 
ZC uses default global TC-LK for NWK key delivery.
ZC allows for requesting TC-LK update. (AIB allowTrustCenterLinkKeyRequests  attribute set to 0x1).

THr1 -TH ZR joining the centralized network at the DUT
THr1 allows for usage of default global TC-LK for joining
This role can be performed by a TH or a golden unit


Test preparation:
Preparatory steps:
P1 - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network. 
P2 - Network steering is triggered on THc1. DUT is powered on. Network steering is triggered on the DUT. DUT successfully joins the centralized network at THc1, incl. successful TC-LK update and verification.
P3 - The last-used value of DUT’s APS Frame counter is known (M).
P4 - Network is closed (AssociationPermit of both THc1 and DUT is set to FALSE, e.g. because it expires). - Redundant


Test procedure:
 - Turn on THr1 and trigger network steering on it.
 




