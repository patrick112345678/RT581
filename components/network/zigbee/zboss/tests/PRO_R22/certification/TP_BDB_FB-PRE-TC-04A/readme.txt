5.1.6 FB-PRE-TC-04A: Service discovery - server side tests 
This test verifies the reception of discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the service discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the service discovery request primitives mandated by the BDB specification.


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.

Test preparation:
P1 - DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE).

Test Procedure:
Reception of Active_EP_req command
1	Unicast Active_EP_req:
THr1 unicasts Active_EP_req command to the network address of the DUT, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00 APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to the network address of the DUT.	DUT sends Active_EP_rsp with Status SUCCESS: 
DUT unicasts Active_EP_rsp command to the network address of the THr1, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present and having any value, 
ZDP Transaction Sequence Number present and having the same value as the triggering request, 
Status = SUCCESS, 
NWKAddrOfInterest field set to the network address of the DUT; 
ActiveEPCount field present and carrying a value as  indicated in the DUT’s PICS; ActiveEPList present if ActiveEPCount !=0x00 and listing all the active application endpoints in an ascending order as indicated in the DUT’s PICS.

Reception of Match_Desc_req command
2	Matching ProfileID:
THr1 unicasts Match_Desc_req command to the DUT, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x0006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, 
the payload fields set as follows: 
NWKAddrOfInterest set to the short address of the DUT, 
ProfileID = 0x0104 (Common), 
NumInClusters = 0x03, InClusterList including: 
exactly one input cluster supported by the DUT (if any), preferably on one DUT endpoint only, 
and remaining input clusters NOT supported by the DUT,
NumOutClusters = 0x03, OutClusterList including: 
exactly one output cluster supported by the DUT (if any), preferably on one DUT endpoint only, 
and remaining output clusters NOT supported by the DUT; e.g. some reserved public clusters .	DUT responds with Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly protected Match_Desc_rsp with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present and set to the ZDP TSN from the triggering request, 
and the payload fields set as follows: 
Status = SUCCESS, 
NWKAddrOfInterest set to the short address of the DUT, 
MatchList listing all the DUT endpoints implementing the supported input/output cluster, and MatchLength set accordingly.

3	Wildcard ProfileID:
THr1 unicasts a correctly formatted Match_Desc_req command to the DUT, with ProfileID =0xffff, with the cluster lists including exactly one input or output cluster, as supported by the DUT.	DUT responds with Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp with Status = SUCCESS, and the MatchList listing all the DUT endpoints implementing the requested input/output cluster.

4	Conditional on DUT being ZC, ZR or a ZED with macRxOnWhenIdle = TRUE:
Broadcast Match_Desc_req:
THr1 broadcasts to 0xfffd a correctly formatted Match_Desc_req command, with NwkAddrOfInterest = 0xfffd, ProfileID = 0x0104 (common), with the cluster lists including exactly one input or output cluster, as supported by the DUT.

Verification:

1	DUT sends Active_EP_rsp with Status SUCCESS: 
DUT unicasts Active_EP_rsp command to the network address of the THr1, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present and having any value, 
ZDP Transaction Sequence Number present and having the same value as the triggering request, 
Status = SUCCESS, 
NWKAddrOfInterest field set to the network address of the DUT; 
ActiveEPCount field present and carrying a value as  indicated in the DUT’s PICS; ActiveEPList present if ActiveEPCount !=0x00 and listing all the active application endpoints in an ascending order as indicated in the DUT’s PICS.

2	DUT responds with Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly protected Match_Desc_rsp with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present and set to the ZDP TSN from the triggering request, 
and the payload fields set as follows: 
Status = SUCCESS, 
NWKAddrOfInterest set to the short address of the DUT, 
MatchList listing all the DUT endpoints implementing the supported input/output cluster, and MatchLength set accordingly.

3	DUT responds with Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp with Status = SUCCESS, and the MatchList listing all the DUT endpoints implementing the requested input/output cluster.

4	DUT responds with Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp (with Ack. Request sub-field of APS Frame Control set to 0b1), with Status = SUCCESS, NwkAddrOfInterest set to the short address of the DUT, and the MatchList listing all the DUT endpoints implementing the requested input/output cluster.

