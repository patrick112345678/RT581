3.4.4 DN-DNS-TC-03: ZR joining at the forming ZR, DUT: joiner ZR 
Objective: This test verifies the extending of a distributed network. 
           The DUT is the ZR attempting to join the distributed network, the test verifies its ability to join a
           distributed network, incl. handling distributed security mode.
           The device under test has to take the role as defined in its PICS, the remaining roles can be performed
           by golden units/test harness, as specified below.

Devices:
THR1 - forming Distributed Network
DUT - joining network formed by DUT (GU or TH)


Initial Conditions
1 A packet sniffer shall be observing the communication over the air interface.
2 All devices are factory new and powered off until used.



Preparatory step:
P1 THr1 is turned on and triggered to form a distributed network. Distributed network is successfully formed.
   There are no other devices in THr1’s network.



Test Step:
1a Network steering is triggered on the THr1. 
1b Turn on DUT. Network steering is triggered on the DUT. 
   DUT broadcasts at least one MAC Beacon Request command on the operational channel of THr1.
1c DUT and THr1 complete association. DUT unicasts an Association Request to the THr1.
2a THr1 delivers the NWK key to DUT: 
   - Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THr1
     unicasts to the DUT the APS Transport Key command, unprotected at the NWK level, correctly protected at the APS
     level with key-transport-key derived from the distributed security global link key, i.e. with:
   - command payload: 
     - Key Type = 0x01 (standard network key), 
     - Key Descriptor present and carrying the network key of 16B, Sequence number corresponding to the
       current NWK key, Destination Address being the IEEE address of the DUT, and Source Address = 0xff..ff. 
2b THr1 sends Device_annce.
3a Since DUT is on a distributed network DUT does not send APS Request Key command.
3b DUT extends the commissioning window:
   DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).


Verification:
1a THr1 enables AssociationPermit.
   THr1 broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
1b THr1 responds with a Beacon:
   THr1 unicasts to DUT a Beacon command on the operational channel of the THr1 network, with the AssociationPermit = TRUE.
1c THr1 unicasts an Association Response to DUT, with AssociationStatus = SUCCESS.
2a DUT starts operating at THr1’s network:
   DUT broadcast Device_annce, correctly protected with the network key.
   Wait for at most 15 seconds.
   Within the 15 seconds, DUT starts broadcasting Link Status messages, correctly protected with the network key.
2b THr1 broadcast Device_annce, correctly protected with the network key.
   Wait for at most 15 seconds.
   Within the 15 seconds, THr1 starts broadcasting Link Status messages, correctly protected with the network key.
3a 
3b THr1 re-broadcasts the Mgmt_Permit_Joining_req.
   DUT sets own AssociationPermit to TRUE.
   Note: the behaviour of a device upon sending of Mgmt_Permi_Join upon completed steering is tested
   in CN-NSA-TC-01A and -01B.







