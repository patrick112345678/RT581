3.6.1	Negative test: DN-KTU-TC-01: NWK key for distributed security network protected with dTCLK
This negative test covers rejection of network key for a distributed network, when protected with incorrect link key.
This test verifies the operation of the device attempting to join a distributed network; the other role has to be performed by a test harness.

3.6.1.1	Required devices
DUT - ZR/ZED, capable of joining a distributed network
THr1  - TH ZR, capable of distributed network formation
TH is modified to send the Transport Key command with the NWK key protected with the dTCLK

3.6.1.2	Initial Conditions
Item	Initial Conditions
1	A packet sniffer shall be observing the communication over the air interface.
2	All devices are factory new and powered off until used.

3.6.1.3	Test preparation
Item	Preparatory steps
P1	THr1 is powered on and triggered to form a distributed network. THr1 successfully forms a distributed network on one of the primary channels as supported by the DUT.
P2	DUT is powered on. Network steering is triggered on the DUT. Network steering is triggered on the THr1. DUT successfully completes the Beacon request, Beacon, Association Request, and Association Response with THr1.

3.6.1.4	Test procedure
Test Step:
1.	Within apsSecurityTimeOutPeriod, THr1 sends NWK key to DUT:
Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THr1 unicasts to the DUT an APS Transport Key command:
-	unprotected at the NWK level,
-	protected at the APS layer with the key-transport-key derived from the default global Trust Center link key, i.e. with:
-	with Security sub-field of Frame Control field of APS header set to 0b1;
-	with APS auxiliary header/trailer fields set as follows:
-	sub-fields of the Security Control field set to: Security Level = 0b000, Key Identifier = 0b10 (Key Transport Key), Extended nonce = 0b1,
-	Source address field present and carrying the IEEE address of THr1;
-	Key Sequence Number field absent; MIC present;
-	command payload with Key Type = 0x01 (standard network key) and Source Address = 0xff..ff.

Verification:
1. DUT does NOT send APS Request Key command. DUT does NOT start sending Link Status messages on the THr1 network.
DUT may try again to join the network of THr1, but shall not try more than bdbcMaxSameNetworkRetryAttempts times; then, after association, THr1 sends again APS Transport Key with the NWK key protected by the default global Trust Center link key.
Eventually, since DUT cannot join the network of THr1 and since there are no other suitable networks on the primary channels of the DUT, DUT scans secondary channels.
