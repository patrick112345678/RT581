15.23 CS-TCP-TC-01B: Reception of NWK Leave command for another node. DUT: ZR.

This test verifies the behaviour of the ZR upon reception of a NWK layer Leave command for one
of the devices on its network

Required devices
DUT - ZR
THr1 - TH ZR
THr2 - TH ZR

Initial conditions:
1	A packet sniffer shall be observing the communication over the air interface.

Preparatory steps:
1	The THr1, THr2 and DUT are operational on the network (bdbNodeIsOnANetwork = TRUE). 

Test Procedure:
Reception of NWK Leave for another node with Request = FALSE and Rejoin = FALSE
5	THr2 sends NWK Leave request for THr2 with Request = 0b0 and Rejoin = 0b0:
THr2 broadcasts a NWK Leave command, with MAC source address set to the network address of the THr2, MAC destination address set 0xffff, NWK header source IEEE address field present and carrying the IEEE address of the THr2, NWK header destination address field set to 0xfffd, Radius = 0x01, correctly protected with the NWK key, with the sub-fields of the Options field set as follows: Rejoin = 0b0; Request = 0b0, Remove children = 0b0; Reserved = 0b0.
Immediately thereafter, turn THr2 off.	wip5: For information: DUT removes THr2 from its internal structures.
This will be verified in step 6.
6	THr1 unicasts to the DUT the Mgmt_Lqi_req.	The DUT unicasts to the THr1 the Mgmt_Lqi_rsp; the NeighborTable List field does NOT include the address of the THr2.

Verification:
5. wip5: For information: DUT removes THr2 from its internal structures.
This will be verified in step 6.
6. The DUT unicasts to the THr1 the Mgmt_Lqi_rsp; the NeighborTable List field does NOT include the address of the THr2.

