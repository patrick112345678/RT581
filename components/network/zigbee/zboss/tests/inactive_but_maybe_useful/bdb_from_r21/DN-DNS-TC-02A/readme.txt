3.4.2 DN-DNS-TC-02A: ZED joining at the forming ZR, DUT: forming ZR
Object: This test verifies the extending of a distributed network.
        The test verifies the ability of a ZR that formed a distributed network to add a ZED, incl. transport of the NWK key.
        The device under test has to take the role as defined in its PICS, the remaining roles can be performed by golden
        units/test harness, as specified below.

Devices:
DUT - forming Distributed Network
THe1 - joining network formed by DUT


Initial Conditions
1 A packet sniffer shall be observing the communication over the air interface.
2 All devices are factory new and powered off until used.


Preparatory step:
P1 DUT is turned on and triggered to form a distributed network. Distributed network is successfully formed.
   There are no other devices in DUT’s network.



Test Step:
1a Network steering is triggered on the DUT.
1b Turn on THe1. 
   Network steering is triggered on the THe1: 
   THe1 broadcasts at least one MAC Beacon Request command on the operational channel of DUT.
1c DUT and THe1 complete association:
   THe1 unicasts an Association Request to the DUT.
2a

2b
3a
3



Verification:
1a DUT enables AssociationPermit.
   DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
1b DUT responds with a Beacon:
   DUT unicasts to THe1 a Beacon command on the operational channel of the DUT network,
   with the AssociationPermit = TRUE, the short address of DUT is NOT equal to 0x0000.
1c DUT unicasts an Association Response to THe1, with AssociationStatus = SUCCESS.
2a  DUT delivers the NWK key to THe1:
    Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the THe1,
    DUT unicasts to THe1 the APS Transport Key command,  unprotected at the NWK level, correctly protected at the
    APS level with key-transport-key derived from the distributed security global link key, i.e. with:
    - Security sub-field of Frame Control field of APS header set to 0b1;
    - with APS auxiliary header/trailer fields set as follows: 
      - sub-fields of the Security Control field set to: Security Level = 0b000,
        Key Identifier 0b10 (Key Transport Key), Extended nonce = 0b1, Frame counter present
      - Source address field present and carrying the IEEE address of the DUT.
      - Key Sequence Number field absent;
      - MIC present
    - CommandID = Transport Key (0x05),
    - command payload: 
      - Key Type = 0x01 (standard network key), 
      - Key Descriptor present and carrying the network key of 16B, Sequence number corresponding to the current NWK key,
        Destination Address being the IEEE address of THe1, and Source Address = 0xff..ff. 
2b THe1 sends Device_annce:
   THe1 broadcast Device_annce, correctly protected with the network key.
3 THe1 extends the commissioning window:
  THe1 broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
  Note: the behaviour of a device upon reception of Mgmt_Permi_Join is tested in CN-NSA-TC-01C.









