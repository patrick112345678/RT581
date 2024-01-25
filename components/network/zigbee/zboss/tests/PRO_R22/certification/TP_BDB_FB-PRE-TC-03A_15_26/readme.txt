15.26 Part of FB-PRE-TC-03A: Service discovery - client side tests: Active_EP_req

This test verifies the generation of the service discovery request commands and handling of the
respective responses, if defined.

Required devices
DUT	- Device operational on a network: ZC, ZR, or ZED; supporting generation of service discovery request primitives
THr1 - TH ZR; operational on a network; supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.
THe1 - TH sleeping ZED, supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness

Initial conditions:
1. A packet sniffer shall be observing the communication over the air interface.

Test preparation:
P0	THr1, DUT and THe1 are factory new and off.
P1	THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE), formed by one of them. 
P2	THe1 joins the network at THr1 (bdbNodeIsOnANetwork = TRUE).
P3	Make sure the Binding table of the DUT is empty. 
If DUT uses groupcast binding, make sure the Groups table of THr1 and THe1 is empty. 

Test procedure:
1	Conditional on DUT supporting transmission of Active_EP_req:
In an application-specific manner, DUT is triggered to send Active_EP_req command for the THr1 (ZR).

Note: if the command is sent as part of the DUT’s finding&binding process the THr1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THr1 side, e.g. enabling F&B.
Discovering services of a THe1 – THr1 parent responds
4	Active_EP_req command for a THe1:
Repeat preparation step P4.
In an application-specific manner, DUT is triggered to send Active_EP_req command for the THe1.
THr1 does not participate in this discovery as a F&B target.

Note: if the command is sent as part of the DUT’s finding&binding process the THe1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THe1 side, e.g. enabling F&B.

Discovering services of a THe1 – THe1 responds
7	Active_EP_req command for a THe1:
Repeat preparation step P4.

In an application-specific manner, DUT is triggered to send Active_EP_req command for the THe1.
THr1 does not participate in this discovery as a F&B target.

Note: if the command is sent as part of the DUT’s finding&binding process the THe1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THe1 side, e.g. enabling F&B.

Verification:
1	DUT unicasts an Active_EP_req command to the network address of the THr1:
correctly protected at the NWK level with the network key, 
unprotected at the APS level, i.e. with:
sub-fields of the Frame Control field set to: sub-fields of the Frame Control field set to: Frame Type = 0b00, Delivery Mode = 0b00 (Normal unicast), Security = 0b0, Extended Header Present = 0b0;
Destination Endpoint = 0x00,
ClusterID=0x0005, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00,
APS Counter field present, 
Extended header and auxiliary header absent;
ZDP Transaction Sequence Number present, 
NWKAddrOfInterest field set to the network address of the THr1.

THr1 unicasts to the DUT a correctly formatted Active_EP_rsp with Status = SUCCESS; NWKAddrOfInterest field set to the network address of the THr1, and with ActiveEPList listing at least one endpoint from the range 0x01 – 0xf0.
If DUT subsequently unicasts any message to the THr1, the message contains THr1’s endpoint, as delivered in the Active_EP_rsp. 
4	DUT unicasts an Active_EP_req command to the network address of the THe1 (or THr1), formatted as in step 1, only with NWKAddrOfInterest field set to the network address of the THe1.
THr1 does NOT forward the request to its sleeping child THe1.

THr1 unicasts to the DUT a correctly formatted Active_EP_rsp, with the command payload fields: 
Status = SUCCESS;
 NWKAddrOfInterest field set to the network address of the THe1,
ActiveEPList listing at least one endpoint from the range 0x01 – 0xf0.

If DUT subsequently unicasts any message to the THe1, the message contains THe1’s endpoint, as delivered in the Active_EP_rsp.
7	DUT unicasts an Active_EP_req command to the network address of the THe1 (or THr1), formatted as in step 1, only with NWKAddrOfInterest field set to the network address of the THe1.

THr1 forwards the request to its sleeping child THe1.

THe1 unicasts to the DUT a correctly formatted Active_EP_rsp, with the command payload fields: 
Status = SUCCESS;
NWKAddrOfInterest field set to the network address of the THe1,
ActiveEPList listing at least one endpoint from the range 0x01 – 0xf0.

If DUT subsequently unicasts any message to the THe1, the message contains THe1’s endpoint, as delivered in the Active_EP_rsp.


