7.3.1 CS-KTU-TC-01: Negative test: NWK key for centralized security network protected with TL LK used
This negative test covers rejection of network key for a centralized network, when protected with incorrect link key. 
This test verifies the operation of the device attempting to join a centralized network; the other role has to be performed by a test harness.


Required devices:
DUT - ZR/ZED, capable of joining a centralized network
THc1 - TH ZC, capable of centralized network formation
TH is modified to send the Transport Key command with the NWK key protected with the touchlink preconfigured link key 


Preparatory steps:
P1 - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network on one of the primary channels as supported by the DUT. 
P2 - DUT is powered on. Network steering is triggered on the DUT. 1Network steering is triggered on the THc1. DUT successfully completes the Beacon request, Beacon, Association Request, and Association Response with THc1.


Test Procedure:
 - Within apsSecurityTimeOutPeriod, THc1 sends NWK key to DUT: 
Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THc1 unicasts to the DUT an APS Transport Key command:
unprotected at the NWK level, 
protected at the APS layer with the touchlink preconfigured link key, i.e. with:
with Security sub-field of Frame Control field of APS header set to 0b1;
with APS auxiliary header/trailer fields set as follows: 
sub-fields of the Security Control field set to: Security Level = 0b000, Key Identifier = 0b00 (Data Key), Extended nonce = 0b1, 
Source address field present and carrying the IEEE address of THc1;
Key Sequence Number field absent; MIC present;
command payload with Key Type = 0x01 (standard network key) and Source Address = IEEE address of THc1.

Verification:
 - DUT does NOT send APS Request Key command.
   DUT does NOT start sending Link Status messages on the THc1 network.
   DUT may try again to join the network of THc1, but shall not try more than bdbcMaxSameNetworkRetryAttempts times; then, after association, THc1 sends again APS Transport Key with the NWK key protected by the touchlink preconfigured link key.
   Eventually, since DUT cannot join the network of THc1 and since there are no other suitable networks on the primary channels of the DUT, DUT scans secondary channels.



Additional information:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zr
