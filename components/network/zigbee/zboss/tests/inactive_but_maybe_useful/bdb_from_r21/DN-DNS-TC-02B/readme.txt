3.4.3 ZED joining at the forming ZR; DUT: ZED
Object: This test verifies the extending of a distributed network. 
        The test verifies the ZED’s ability to join a distributed network, incl. handling distributed security mode. 
        The device under test has to take the role as defined in its PICS, the remaining roles can be performed by golden
        units/test harness, as specified below.

Devices:
THr1 - zr forming Distributed Network
DUT - zed joining network formed by DUT


Initial Conditions
1 A packet sniffer shall be observing the communication over the air interface.
2 All devices are factory new and powered off until used.


Preparatory step:
P1 THr1 is turned on and triggered to form a distributed network. Distributed network is successfully formed.
   There are no other devices in THr1’s network.




Test Step:
1a Network steering is triggered on the THr1. 
   Turn on DUT. Network steering is triggered on the DUT. 
1b THr1 responds with a Beacon:
   THr1 unicasts to DUT a Beacon command on the operational channel of the THr1 network, with the AssociationPermit = TRUE,
   the short address of THr1 is NOT equal to 0x0000.
2a THr1 delivers the NWK key to DUT: 
   Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THr1
   unicasts to DUT the APS Transport Key command, unprotected at the NWK level, correctly protected at the APS level
   with key-transport-key derived from the distributed security global link key, command payload: 
   - Key Type = 0x01 (standard network key), 
   - Key Descriptor carrying the network key of 16B,
   - Sequence number corresponding to the current NWK key,
   - Destination Address being the IEEE address of DUT,
   - and Source Address = 0xff..ff.



Verification:
1a DUT broadcasts at least one MAC Beacon Request command on the operational channel of THr1.
1b THr1 and DUT complete association:
   DUT unicasts an Association Request to the THr1. THr1 unicasts an Association Response to DUT, with
   AssociationStatus = SUCCESS.
2a DUT sends Device_annce:
   - DUT broadcast Device_annce, correctly protected with the network key.
   - Since it joined a distributed network, the DUT does NOT initiate TC-LK update.
   DUT extends the commissioning window:
   - DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
   - THr1 re-broadcasts the Mgmt_Permit_Joining_req.
   Note: the behaviour of a device upon sending of Mgmt_Permi_Join upon completed steering is tested in CN-NSA-TC-01A and -01B.

