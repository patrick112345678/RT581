FB-PRE-TC-01A: Device discovery - client side tests
This test verifies the generation of the device discovery request commands mandated by the BDB specification and handling of the respective responses, if defined.

Required devices
DUT	- Device operational on a network, ZC, ZR, or ZED; supporting generation of device discovery request primitives as mandated by the BDB specification
THr1 - TH ZR operational on a network; supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.
THe1 - TH sleeping ZED, supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness

Additional devices
THr2 - TH ZR, Presence of a separate THr2 node required, to prevent DUT adding THr1 to its neighbor table (as a parent) 
This role can be performed by a golden unit or a test harness

Initial conditions:
1	A packet sniffer shall be observing the communication over the air interface.

Preparatory steps:
P0	THr2, THr1, DUT and ZED are factory new and off.
P1	Switch DUT and THr2 on. DUT and THr2 become operational on the same network, formed by one of them.
P2	If DUT is a ZR: switch DUT off.
THr1 joins the network at THr2 (bdbNodeIsOnANetwork = TRUE). 
P3	If DUT is a ZR: switch the DUT back on.

Test Procedure:
1a	Repeat preparation steps P0-P2.
1b	IEEE_addr_req command for a ZR:
In an application-specific manner, DUT is triggered to send IEEE_addr_req command for the THr1.

Note: if the command is sent as part of the DUT’s finding&binding process the THr1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THr1 side, e.g. enabling F&B.
2a	Switch DUT off.
THe1 joins at the THr1, out of radio range of DUT.
Switch DUT on.
2b	IEEE_addr_req command for a THe1:
In an application-specific manner, DUT is triggered to send IEEE_addr_req command for the THe1.

Note: if the command is sent as part of the DUT’s finding&binding process the THe1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THe1 side, e.g. enabling F&B.
2c	Conditional on DUT sending IEEE_addr_req with RequestType = 0b01: 
THr1 unicasts to the DUT a correctly formatted IEEE_addr_rsp, with the command payload fields: 
Status = SUCCESS;
 IEEEAddrRemoteDev = IEEE address of the THe1;
NWKAddrRemoteDev = network address of the THe1; 
the fields NumAssocDev = 0x00,  StartIndex = 0x00, NWKAddrAssocDevList absent.
2d	Conditional on DUT sending IEEE_addr_req with RequestType = 0b00: 
THr1 forwards DUT’s IEEE_addr_req to the THe1.
THe1 unicasts to the DUT a correctly formatted IEEE_addr_rsp, with the command payload fields: 
Status = SUCCESS;
 IEEEAddrRemoteDev = IEEE address of the THe1;
NWKAddrRemoteDev = network address of the THe1; 
the fields NumAssocDev,  StartIndex, and NWKAddrAssocDevList absent.

Verification:
1b	DUT unicasts to the THr1 a IEEE_addr_req command: 
correctly protected at the NWK level with the network key, 
unprotected at the APS level, i.e. with APS header:
sub-fields of the Frame Control field set to: Frame Type = 0b00, Delivery Mode = 0b00 (Normal unicast), Security = 0b0, Extended Header Present = 0b0;
Destination Endpoint = 0x00,
ClusterID=0x0001, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, 
command payload with: 
NwkAddrOfInterest field set to the NWK address of the THr1, 
Request Type set to either 0x00 (single response) or 0x01 (extended response), 
StartIndex = 0x00.

THr1 unicasts to the DUT a correctly formatted IEEE_addr_rsp with Status = SUCCESS; IEEEAddrRemoteDev = IEEE address of the THr1; NWKAddrRemoteDev = network address of the THr1.

If DUT subsequently unicasts any message to the THr1 that also contains THr1’s IEEE address, the THr1’s IEEE address has a value as delivered in the IEEE_addr_rsp.
2a	
2b	DUT unicasts to network address of the THe1 the IEEE_addr_req command:
correctly protected with the network key,
unprotected at the APS level, i.e. with APS header: 
sub-fields of the Frame Control field set to: Frame Type = 0b00, Delivery Mode = 0b00 (Normal unicast), Security = 0b0, Extended Header Present = 0b0;
Destination Endpoint = 0x00,
ClusterID=0x0000, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present, 
Extended header and auxiliary header absent
ZDP Transaction Sequence Number present, 
command payload with: 
NetworkAddressOfInterest field set to the NWK address of the THe1, 
Request Type set to either 0x00 (single response) or 0x01 (extended response), 
StartIndex = 0x00.

THr1 does NOT forward the request to its sleeping child THe1.
2c	If DUT subsequently unicasts any message to the THe1 that also contains THe1’s IEEE address, the THe1’s IEEE address has a value as delivered in the IEEE_addr_rsp.
Alternatively, check DUT’s binding table or nwkAddressMap attribute of the NIB.

2d	If DUT subsequently unicasts any message to the THe1 that also contains THe1’s IEEE address, the THe1’s IEEE address has a value as delivered in the IEEE_addr_rsp.
Alternatively, check DUT’s binding table or nwkAddressMap attribute of the NIB.




