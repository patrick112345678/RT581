4.5.5 CN-NSA-TC-03: Looping after joining failure; DUT: joiner (ZR/ZED)
This test covers looping behavior during network steering on a factory-new device, i.e. its ability to try joining another network if joining of the previous network failed. 
This test verifies the operation of the ZED/ZR that attempts to join a network; the other role can be performed by a test harness.
While the test checks looping upon unsuccessful joining of a centralized network, because there are more failure causes, the looping behavior itself is independent of the network type/security mode.


Required devices:
DUT - ZR/ZED, attempting to join a network

THc1 - TH ZC operational on a centralized network 
       this role shall be performed by a TH

THr1 - TH ZR operational on a centralized network of THc1 
       this role shall be performed by a TH
       
THc2 - TH ZC, operational on a centralized network 
       this role shall be performed by a  TH


Preparatory steps:
P0 - All devices are factory new and powered off until used.
P1 - The value of DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet are known. Neither is 0x00000000.
P2a - THc1 is powered on and triggered to form a centralized network (network A). 
      THc1 successfully forms a centralized network on a primary channel supported by the DUT. 
P2b - THr1 is powered on and triggered to join the centralized network of THc1. It successfully joins network A, incl. TC-LK update and verification.
P2c - THc2 is powered on and triggered to form a centralized network (network B). 
      THc2 successfully forms a centralized network on a primary channel supported by the DUT, the same as used by THc1. There is no other device in THc2’s network.
P3 - Network steering is triggered on THc1, and THc2.
     Note: the test steps have to be performed while the networks of THc1 and THc2 are open for joining. 
P4 - DUT is placed within radio range of THc1, THr1 and THc2.


Test procedure:
 - No child capacity at THc1
   - DUT is powered on. Network steering is triggered on the DUT
   - Since THc1’s network was formed on a primary channel supported by the DUT, THc1 and THr1 respond with a Beacon.
     THc1 responds with Beacon, indicating no child capacity:
     THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but with Router capacity and the End device capacity field of the beacon payload both set to FALSE.
     THr1 responds with Beacon frame:
     THr1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; and with Router capacity and the End device capacity field of the beacon payload both set to TRUE.

 - Incomplete association at one node of network A
   - DUT is powered on. Network steering is triggered on the DUT.
   - Since THc1’s network was formed on a primary channel supported by the DUT, both THc1 and THr1 respond with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.
   - The candidate parent node on network A (THc1/THr1) does NOT answer: The THc1/THr1 (as selected by the DUT) does NOT send Association Response.
   - Since the initially selected candidate parent does not answer, DUT initiates joining to network A at the other candidate parent.

 - Incomplete association in network A
   - DUT is powered on. Network steering is triggered on the DUT.
   - Since THc1’s network was formed on a primary channel supported by the DUT, both THc1 and THr1 respond with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.
   - The candidate parent nodes on network A (THc1 and THr1) do NOT answer:
The THc1/THr1 (as selected by the DUT) does NOT send Association Response.
   - Since the candidate parents on network A do not answer, and since there are no other suitable open networks on the DUT’s primary channels, DUT initiates a scan on DUT’s secondary channels.
   - Since THc2’s network was formed on a secondary channel supported by the DUT, both THc1 and THr1 respond with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.

 - Network key not delivered
   - DUT is powered on. Network steering is triggered on the DUT.
   - Since THc1’s network was formed on a primary channel supported by the DUT, THc1 responds with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.
   - Within apsSecurityTimeOutPeriod after successful reception of MAC Association Response, THc1 does NOT send a NWK key to the DUT.
   - Since THc1 does not provide DUT with the network key, and since there is no other suitable open network on the DUT’s primary channels, DUT performs active scan on the secondary channels.
   - Since THc2’s network was formed on a secondary channel supported by the DUT, THc2  responds with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.

 - Unique TCLK not delivered
   - DUT is powered on. Network steering is triggered on the DUT.
   - Since THc1’s network was formed on a primary channel supported by the DUT, THc1 responds with a Beacon frame with AssociationPermit = TRUE; all beacon and beacon payload fields are correct.
   - THc1 does not send to the DUT the APS Transport Key command with unique TC-LK for the DUT.
   - Since THc1 is r21 TC, but does NOT provide updated TCLK, DUT removes itself from network A.



Additional info:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zc
