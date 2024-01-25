3.4.5 DN-DNS-TC-03: DN-DNS-TC-04: Joining at the joined ZR, DUT: parent ZR
Objective: This test verifies extending a distributed network, specifically the role of the ZR device already on the
           distributed network, incl. its ability to pass on the network key.
           The device under test has to take the role as defined in its PICS, the remaining roles can be performed by
           golden units/test harness, as specified below.
           Successfult completion of test 3.4.4 is prerequisite for performing the current test. If the current test
           immediately follows on test 3.4.4, the preparation steps of the current test can be omitted.

Required Devices:
THr1 - TH ZR, capable of distributed network formation a golden unit device / TH
DUT - ZR, capable of joining distributed network 
THr2 - TH ZR, capable of joining distributed network a golden unit device / TH 



Initial Conditions
1 A packet sniffer shall be observing the communication over the air interface.
2 All devices are factory new and powered off until used.



Preparatory step:
P1 THr1 is turned on and triggered to form a distributed network. Distributed network is successfully formed.
P2 Network steering is triggered on THr1. DUT is brought in-range of THr1, powered on and network steering is
   triggered on the DUT. DUT successfully joins the distributed network of THr1. Make sure the permit join expires.
P3 Switch THr1 off.



Test Step:
1a Network steering is triggered on the DUT.
1b THr2 is powered on. Network steering is triggered on the THr2.
1c THr2 does active scan on primary and optionally secondary channels.
   THr2 broadcasts at least one MAC Beacon Request command on the operational network of the DUT.
1d THr2 successfully associates with the DUT:
   - THr2 unicasts an Association Request to the DUT.
2a DUT delivers the NWK key to the THr2. 
2b THr2 starts operating on DUT’s network
3a THr2 extends commissioning window: THr2 broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of
   at least bdbcMinCommissioningTime (180s).



Verification:
1a DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
   DUT sets own AssociationPermit to TRUE.
1b
1c DUT unicasts to the THr2 a Beacon command, with the AssociationPermit = TRUE.
1d DUT unicasts an Association Response to THr2, with AssociationStatus = SUCCESS.
2a Since the DUT is on a distributed network, it does NOT send APS Update device command.
   Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the THr2, DUT unicasts
   to the THr2 the APS Transport Key command: 
   - unprotected at the NWK level,
   - correctly protected at the APS level with a key-transport-key derived from the distributed security global link key 
   - with Security sub-field of Frame Control field of APS header set to 0b1;
   - with APS auxiliary header/trailer fields set as follows: 
     - sub-fields of the Security Control field set to: Security Level = 0b000, Key Identifier = 0b10 (Key Transport Key),
       Extended nonce = 0b1, 
     - Frame counter present
     - Source address field present and carrying the IEEE address of DUT.
     - Key Sequence Number field absent;
     - MIC present
   - CommandID = Transport Key (0x05), command payload: 
     - Key Type = 0x01 (standard network key), 
     - Key Descriptor present and carrying: the network key of 16B, Sequence number corresponding to the current NWK key,
       Destination address being the IEEE address of THr2, and Source Address = 0xff..ff,.
2b THr2 broadcast Device_annce, correctly protected with the network key.
3a DUT re-broadcasts Mgmt_Permit_Joining_req. 
   Note: the behaviour of a device upon reception of Mgmt_Permi_Join is tested in CN-NSA-TC-01C.


