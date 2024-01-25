4.5.4 N-NSA-TC-02: Network steering – looping, Network selection tests; DUT: joiner (ZR/ZED)
This test covers looping behavior during network steering on a factory-new device, i.e. its ability to try joining another network if joining of the previous network failed. The test checks the joiner’s behavior upon reception of all sorts of incorrectly formed Beacon commands.
This test verifies the operation of the ZED/ZR that attempts to join a network; the other role can be performed by a test harness.
While the test checks looping upon unsuccessful joining of a centralized networks, because there are more failure causes, the looping behavior itself is independent of the network type/security mode.


Required devices:
DUT - ZR/ZED, attempting to join a network

Additional devices:
THc1 - TH ZC, operational on a centralized network;
THc1 has to be capable of sending all sorts of incorrect Beacon commands;
this role shall be performed by a TH

THc2 - TH ZC, operational on a centralized network
THc2 has to be capable of sending all sorts of incorrect Beacon commands 
this role shall be performed by a TH

THc3 - ZC, operational on a centralized network 
this role can be performed by a golden unit or a TH


Test preparation:
P0 - The value of DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet are known. Neither is 0x00000000.
P1 - All devices are factory new and powered off until used.
P2a - THc1 is powered on and triggered to form a centralized network. 
THc1 successfully forms a centralized network on a channel of the DUT’s bdbPrimaryChannelSet. 
There is no other device in THc1’s network.
P2b - THc2 is powered on and triggered to form a centralized network. 
 on the same channel as used by THc1.
There is no other device in THc2’s network.
P2c - THc3 is powered on and triggered to form a centralized network. 
THc3 successfully forms a centralized network on a channel of the DUT’s bdbSecondaryChannelSet
There is no other device in THc3’s network.
P2d - There is no other open network on the DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet.
P3 - Network steering is triggered on THc1, THc2 and THc3.
P4 - DUT is placed within radio range of THc1, THc2 and THc3.
Note: the test steps have to be performed while the networks of THc1-THc3 are open for joining.


Test Procedure:
 => No open network on the primary channels:
    Note: the test steps 1-3 have to be performed while the network of THc3, opened in preparatory step P3, are still open for joining.
   - THc1 is powered off.
   - Network steering is stopped on THc2.
   - DUT is powered on.
   - Network steering is triggered on the DUT.
   
 => Incorrect values in the beacon payload:
    THc1 is powered back on.
    Preparation steps P1 – P3 are repeated.
    Note: the test steps 4-5 have to be performed while the networks of THc1-THc3, opened in preparatory step P3, are still open for joining.
    - DUT is powered on. Network steering is triggered on the DUT.
    - THc1 responds with Beacon frame with incorrect payload:
      THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but with ProtocolID field of the beacon payload set to value other than 0x00 and 0xff (e.g. 0x01).
      THc2 responds with Beacon frame with incorrect payload:
      THc2 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but with Stack profile field of the beacon payload set to value other than 0x01 and 0x02 (e.g. 0x03).
      
 => Incorrect values in the beacon payload (2):
    Preparation steps P1 – P3 are repeated.
    Note: the test steps 6-7 have to be performed while the networks of THc1-THc3, opened in preparatory step P3, are still open for joining.
    - DUT is powered on. Network steering is triggered on the DUT.
    - THc1 responds with Beacon frame with incorrect payload:
      THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but with nwkcProtocolVersion field of the beacon payload set to value other than 0x0 – 0x3 (e.g. 0x8).
      THc2 responds with Beacon frame with incorrect payload:
      THc2 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but with Router capacity and the End device capacity field of the beacon payload both set to FALSE.
      
 => Too short beacon payload
    Preparation steps P1 – P3.
    Note: the test steps 8-9 have to be performed while the networks of THc1-THc3, opened in preparatory step P3, are still open for joining.
    - DUT is powered on. Network steering is triggered on the DUT.
    - THc1 responds with Beacon frame with only 2B of payload:
      THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but the beacon payload contains only the initial 2 octets.
      THc2 responds with Beacon frame with only 11B of payload:
      THc2 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; but the beacon payload contains only the initial 11 octets.

 => Too long beacon:
    Preparation steps P1 – P2a are repeated (TH-THc2 and TH-THc3 stay off).
    Enable network steering on THc1.
    Note: the test steps 10-11 have to be performed while the network of THc1, opened in preparatory step P3, is still open for joining.
    - DUT is powered on. Network steering is triggered on the DUT.
    - THc1 responds with Beacon frame with 20B of payload:
    - THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; at the end of the beacon payload, 5 additional octets are appended.

 => Reserved fields set in the beacon payload
    Preparation steps P1 – P2a are repeated (TH-THc2 and TH-THc3 stays off).
    Enable network steering on THc1.
    Note: the test steps 12 - 13 have to be performed while the networks of THc1 is open for joining.
    - DUT is powered on. Network steering is triggered on the DUT.
    - THc1 responds with Beacon frame with the Reserved fields of the payload set:
      THc1 unicasts to the DUT a Beacon frame with AssociationPermit = TRUE; the Reserved bits (b16 and b17) are set to TRUE.


Additional info:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zc
