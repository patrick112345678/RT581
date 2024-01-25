CN-NST-TC-04: joining via ZR; DUT: joiner (ZR or ZED)

This test verifies the network steering procedure of a device joining one hop away from the ZC. ZR or ZED can be the DUT. 
This test verifies joiner’s behavior during multi-hop joining of a centralized network, incl. NWK key delivery and TCLK update.  
The device takes the role described in its PICS; the other role can be performed by a golden unit or test harness, as indicated below.


Required devices:
DUT - ZED or ZR, joining the centralized network at THr1
DUT allows for usage of default global TC-LK for joining

THc1 - TH ZC of the network; 
ZC uses default global TC-LK for NWK key delivery.

THr1 - TH ZR joining the centralized network at the ZC, and then becoming a parent itself
ZR allows for usage of default global TC-LK for joining
this role can be performed by a TH or a golden unit


Test preparation:
P1 - THc1 is powered on. Network formation is triggered on the THc1. THc1 successfully forms a centralized network.
P2 - Network steering is triggered on the THc1. THr1 is powered on and network steering is triggered on the THr1. THr1 successfully joins centralized network of the THc1, incl. TC-LK update.


Test procedure:
 - Trigger network steering on the DUT
