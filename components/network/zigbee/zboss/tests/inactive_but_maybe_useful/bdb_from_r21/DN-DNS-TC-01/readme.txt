3.4.1 DN-DNS-TC-01: ZR joining at the forming ZR, DUT: forming ZR
Objective: This test verifies the extending of a distributed network.
           The DUT is the ZR that formed the distributed network, the test verifies its ability to add a
           further router, incl. transmitting the NWK key. 
           The device under test has to take the role as defined in its PICS, the remaining roles can be
           performed by golden units/test harness, as specified below.


Devices:
DUT - forming Distributed Network
THZR1 - joining network formed by DUT (golden unit (GU) ot test harness(TH))


Initial Conditions
1 A packet sniffer shall be observing the communication over the air interface.
2 All devices are factory new and powered off until used.


Preparatory step:
 - DUT is turned on and triggered to form a distributed network. Distributed network is successfully formed.
   There are no other devices in DUT’s network.




Test Step (Before):
1a Network steering is triggered on the DUT.
1b Wait for approx. 120 seconds (to test the Permit Duration).
   Turn on THr1. Network steering is triggered on the THr1. 
   THr1 broadcasts at least one MAC Beacon Request command on the operational channel of DUT.
1c DUT and THr1 complete association:
   THr1 unicasts an Association Request to the DUT.
2a DUT delivers the NWK key to THr1. 
2b THr1 sends Device_annce.
3a THr1 extends the commissioning window:
   THr1 broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
3 Verify step 1a.
  Note: a device has joined the DUT’s network and thus the NWK key was sent over the air, making it possible for
  the sniffer to decrypt the protected message transmitted by the DUT after network formation. They can be verified now.


Verification (After):
1a DUT enables AssociationPermit.
   DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
   Note: since no device joined the DUT’s network, the sniffer may not be able to decrypt this message at this point.
   This will be verified in step 3c.
1b DUT responds with a Beacon:
   DUT unicasts to THr1 a Beacon command on the operational channel of the DUT network, with the
   AssociationPermit = TRUE, the short address of DUT is NOT equal to 0x0000.
1c DUT unicasts an Association Response to THr1, with AssociationStatus = SUCCESS.
2a Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the THr1, DUT unicasts
   to THr1 the APS Transport Key command, unprotected at the NWK level, correctly protected at the APS level with
   key-transport-key derived from the distributed security global link key, i.e. with:
   - Security sub-field of Frame Control field of APS header set to 0b1;
   - with APS auxiliary header/trailer fields set as follows: 
     - sub-fields of the Security Control field set to: Security Level = 0b000,
       Key Identifier 0b10 (Key Transport Key), Extended nonce = 0b1, 
     - Frame counter present
     - Source address field present and carrying the IEEE address of the DUT.
     - Key Sequence Number field absent;
     - MIC present
   - CommandID = Transport Key (0x05),
   - command payload: 
     - Key Type = 0x01 (standard network key), 
     - Key Descriptor present and carrying the network key of 16B, Sequence number corresponding to the
       current NWK key, Destination Address being the IEEE address of THr1, and Source Address = 0xff..ff. 
2b THr1 broadcast Device_annce, correctly protected with the network key.
   Wait for at most 15 seconds.
   Within the 15 seconds, THr1 starts broadcasting Link Status messages, correctly protected with the network key.
3a DUT re-broadcasts the Mgmt_Permit_Joining_req.
Note: the behaviour of a device upon reception of Mgmt_Permi_Join is tested in CN-NSA-TC-01C.
3 DUT did send the message of step 1a, formatted exactly as described there.







