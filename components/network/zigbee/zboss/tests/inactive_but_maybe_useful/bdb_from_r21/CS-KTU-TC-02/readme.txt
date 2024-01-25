7.3.2 CS-KTU-TC-02: Negative test: NWK key for centralized security network protected with distributed security global link key
This negative test covers rejection of network key for a centralized network, when protected with incorrect link key. 
This test verifies the operation of the device attempting to join a centralized network; the other role has to be performed by a test harness.


Required devices:
DUT - ZR/ZED, capable of joining a centralized network
THc1 - TH ZC, capable of centralized network formation
TH is modified to send the Transport Key command with the NWK key protected with the distributed security global link key


Preparatory steps:
P1 - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network on one of the primary channels as supported by the DUT. 
P2 - DUT is powered on. Network steering is triggered on the DUT. Network steering is triggered on the THc1. DUT successfully completes the Beacon request, Beacon, Association Request, and Association Response with THc1.



Test Procedure:
 - Within apsSecurityTimeOutPeriod, THc1 sends NWK key to DUT: 
Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THc1 unicasts to the DUT an APS Transport Key command:
unprotected at the NWK level, 
protected at the APS layer with the KTK derived from the distributed security global link key, i.e. with:
with Security sub-field of Frame Control field of APS header set to 0b1;
with APS auxiliary header/trailer fields set as follows: 
sub-fields of the Security Control field set to: Security Level = 0b000, Key Identifier = 0b10 (Key Transport Key), Extended nonce = 0b1, 
Source address field present and carrying the IEEE address of THc1;
Key Sequence Number field absent; MIC present;
command payload with Key Type = 0x01 (standard network key) and Source Address = IEEE address of THc1.



Additional information:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zr
