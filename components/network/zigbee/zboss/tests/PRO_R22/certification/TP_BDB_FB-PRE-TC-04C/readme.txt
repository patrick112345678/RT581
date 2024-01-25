5.1.6 FB-PRE-TC-04A: Service discovery - server side tests 
This test verifies the reception of discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the service discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the service discovery request primitives mandated by the BDB specification.


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.

Test preparation:
P1	DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE) formed by one of them.
P2	An active endpoint of the DUT is known.

Test procedure:
1	Unicast Simple_Desc_req:
THr1 unicasts Simple_Desc_req command to the network address of the DUT, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x0004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present,
NWKAddrOfInterest field set to the network address of the DUT, 
and EndPoint field set to an active endpoint of the DUT.

Verification:
1. DUT sends Simple_Desc_rsp with Status SUCCESS:
DUT unicasts Simple_Desc_rsp command to the network address of the THr1:
correctly protected with the network key;
with fields of the APS header: 
sub-fields of the Fame Control: Frame Type  = 0b00, 
Destination Endpoint = 0x00, ClusterID = 0x8004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present and having any value, 
ZDP Transaction Sequence Number present and having the same value as the triggering request, 
Command payload with
Status = SUCCESS,
NWKAddrOfInterest field set to the network address of the DUT;
Length field present and set to the length in octets of the following simple descriptor, 
the Simple Descriptor field present and its subfields set to:
Endpoint: the same value as in the triggering request, 
Application profile identifier = 0x0104; 
Application device identifier – according to DUT’s PICS for that endpoint, 
Application device version – according to DUT’s PICS (as defined by device specification),
 Reserved bits set to 0b0,
Application input cluster count matching the number of clusters in the Application input cluster list;
Application input cluster list listing all server clusters in DUT’s PICS;
Application output cluster count matching the number of clusters in the Application output cluster list;
Application output cluster list listing all client clusters in DUT’s PICS.

