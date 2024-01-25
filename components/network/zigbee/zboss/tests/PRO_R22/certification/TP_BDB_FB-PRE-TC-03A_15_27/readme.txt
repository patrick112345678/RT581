15.27 Part of FB-PRE-TC-03A: Service discovery - client side tests: Match_desc_req

This test verifies the generation of the service discovery request commands and handling of the
respective responses, if defined.

Required devices
DUT - Device operational on a network: ZC, ZR, or ZED; supporting generation of service discovery request primitives
THr1 - TH ZR; operational on a network; supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.
THe1 - TH sleeping ZED, supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness

Initial conditions:
1	A packet sniffer shall be observing the communication over the air interface.

Test preparation:
P0	THr1, DUT and THe1 are factory new and off.
P1	THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE), formed by one of them. 
P2	THe1 joins the network at THr1 (bdbNodeIsOnANetwork = TRUE).
P3	Make sure the Binding table of the DUT is empty. 
If DUT uses groupcast binding, make sure the Groups table of THr1 and THe1 is empty. 

Test procedure:
2	Conditional on DUT supporting transmission of Match_Desc_req:
Repeat preparation step P3.
Finding&binding in the initiator role is triggered on the DUT.
In an application-specific manner, DUT is triggered to send Simple_Desc_req command for the THr1.

Note: if the command is sent as part of the DUT’s finding&binding process the THr1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THr1 side, e.g. enabling F&B.
Discovering services of a THe1 – THe1 responds
6	Conditional on DUT supporting transmission of Match_Desc_req:
If required, repeat preparation step P3.
Finding&binding in the initiator role is triggered on the DUT.
In an application-specific manner, DUT is triggered to send Simple_Desc_req command for the THe1.
THr1 does not participate in this discovery as a F&B target.

Note: if the command is sent as part of the DUT’s finding&binding process the THe1 shall respond to any preceding discovery commands in such a way, that the process successfully continues. This may require some actions on the THe1 side, e.g. enabling F&B.

Verification:
2	DUT unicasts to the network address of the THr1 or broadcasts Match_Desc_req command:  
correctly protected at the NWK level with the network key, 
unprotected at the APS level, i.e. with:
sub-fields of the Frame Control field set to: sub-fields of the Frame Control field set to: Frame Type = 0b00, Delivery Mode = 0b00 (Normal unicast), Security = 0b0, Extended Header Present = 0b0;
Destination Endpoint = 0x00,
ClusterID=0x0006, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00,
APS Counter field present, 
Extended header and auxiliary header absent;
ZDP Transaction Sequence Number present, 
payload
NWKAddrOfInterest set to:
NWK address of THr1, if the request was unicast;
0xffff, if the request was broadcast;
ProfileID = 0x0104 (Common) or 0xffff;
At least one of InClusterList and OutClusterList listing at least one cluster from DUT’s PICS;
NumInClusters and NumOutClusters set accordingly.

THr1 unicasts to the DUT a correctly formatted Match_Desc_rsp with Status = SUCCESS, NWKAddrOfInterest set to the short address of the THr1 MatchList listing one endpoint of the THr1 supporting at least one of the required input/output clusters.

THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00, to verify, based on DUT’s Mgmt_Bind_rsp, that:
DUT establishes a unicast or groupcast binding for at least one matching cluster of the THr1’s endpoint.
6	DUT unicasts to the network address of the THe1 or broadcasts Match_Desc_req command, correctly formatted as in step 2, only with:  
NWKAddrOfInterest set to:
NWK address of THe1, if the request was unicast;
0xffff, if the request was broadcast;
 ProfileID = 0x0104 (Common),
At least one of InClusterList and OutClusterList listing at least one cluster from DUT’s PICS;
NumInClusters and NumOutClusters set accordingly.

THr1 forwards the request to its sleeping child THe1.

THe1 unicasts to the DUT a correctly formatted Match_Desc_rsp with Status = SUCCESS, NWKAddrOfInterest set to the short address of the THe1 MatchList listing one endpoint of the THe1 supporting at least one of the required input/output clusters.

THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00, to verify, based on DUT’s Mgmt_Bind_rsp, that:
DUT establishes a unicast or groupcast binding for at least one matching cluster of the THe1’s endpoint.


