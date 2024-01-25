5.1.7 FB-PRE-TC-04B: Service discovery - server side additional tests 
This test contains negative checks for reception of discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the service discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the service discovery request primitives mandated by the BDB specification.


Initial conditions
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P1 - DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE).

Test procedure:

Reception of Active_EP_req command
1a	Negative test: Unicast Active_EP_req for unknown device:
THr1 unicasts to the DUT the Active_EP_req for a non-existent device. 

1b	Conditional on 1a and DUT being a ZR/ZC:
DUT sends Active_EP_rsp with Status DEVICE_NOT_FOUND.

1c	Conditional on 1a and DUT being a ZED:
DUT sends Active_EP_rsp with Status INV_REQUESTTYPE.

Reception of Simple_Desc_req command
3a	Negative test: Unicast Simple_Desc_req for unknown device:
THr1 unicasts to the DUT the Simple_Desc_req for a non-existent device. 

3b	Conditional on 3a and DUT being a ZR/ZC:
DUT sends Simple_Desc_rsp with Status DEVICE_NOT_FOUND.

3c	Conditional on 3a and DUT being a ZED:
DUT sends Simple_Desc_rsp with Status INV_REQUESTTYPE.

4	Negative test: Unicast Simple_Desc_req for inactive endpoint:
THr1 unicasts to the short address of the DUT a Simple_Desc_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to a short address of the DUT, and the EndPoint field to a value from the 0x01 – 0xf0 range for which the DUT doesn’t have an active endpoint.

6	Negative test: Incorrect endpoint:
THr1 unicasts to the short address of the DUT a Simple_Desc_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to a short address of the DUT, and the EndPoint field set to 0xff.

Reception of Match_Desc_req command
9a	Negative test: unsupported ProfileID & unicast:
THr1 unicasts to the DUT a correctly formatted Match_Desc_req command, with NwkAddrOfInterest set to the short address of the DUT, ProfileID NOT supported by the DUT (e.g.0x0110), with the cluster lists including at least one cluster supported by the DUT.

9b	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: unsupported ProfileID & broadcast:
THr1 broadcasts to 0xfffd a correctly formatted Match_Desc_req command, with NwkAddrOfInterest set to the short address of the DUT, ProfileID NOT supported by the DUT (e.g.0x0110), with the cluster lists including at least one cluster supported by the DUT.

10a	Negative test: no matching cluster & unicast:
THr1 unicasts to the DUT a correctly formatted Match_Desc_req command, with NwkAddrOfInterest set to the short address of the DUT, ProfileID = 0x0104 (common), with the cluster lists including a number of clusters none of which is supported by any endpoint of the DUT.

10b	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: no matching cluster & broadcast:
THr1 broadcasts to 0xfffd a correctly formatted Match_Desc_req command, with NwkAddrOfInterest set to the short address of the DUT, ProfileID = 0x0104 (common), with the cluster lists including a number of clusters none of which is supported by any endpoint of the DUT.

11a	Negative test: Unicast Match_Desc_req for unknown device:
THr1 unicasts to the DUT the Match_Desc_req for a non-existent device. 

11b	Conditional on 11a and DUT being a ZR/ZC:
DUT sends Match_Desc_rsp with Status DEVICE_NOT_FOUND.

11c	Conditional on 11a and DUT being a ZED:
DUT sends Match_Desc_rsp with Status INV_REQUESTTYPE.

12a	Negative test: no cluster list & unicast:
THr1 unicasts Match_Desc_req command to the DUT, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, and the payload fields set as follows: NWKAddrOfInterest set to the short address of the DUT, ProfileID = 0x0104  (Common), NumInClusters = NumOutClusters = 0x00, and InClusterList and OutClusterList not included.

12b	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: no cluster list & broadcast:
THr1 broadcasts to 0xfffd Match_Desc_req DUT, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, and the payload fields set as follows: NWKAddrOfInterest set to the short address of the DUT, ProfileID = 0x0104 (Common), NumInClusters = NumOutClusters = 0x00, and InClusterList and OutClusterList not included.

Verification:
1a	THr1 unicasts to the short address of the DUT an Active_EP_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to a short address NOT present in the network.
1b	DUT unicasts Active_EP_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = DEVICE_NOT_FOUND, NWKAddrOfInterest field set to the value of the same field in the triggering request; ActiveEPCount field present and set to 0x00; ActiveEPList absent.
1c	DUT unicasts Active_EP_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8005, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = INV_REQUESTTYPE, NWKAddrOfInterest field set to the value of the same field in the triggering request; ActiveEPCount field present and set to 0x00; ActiveEPList absent.
3a	THr1 unicasts to the short address of the DUT a Simple_Desc_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to a short address NOT present in the network, and the EndPoint field set to one of the active endpoints of the DUT.
3b	DUT unicasts Simple_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = DEVICE_NOT_FOUND, NWKAddrOfInterest field set to the value of the same field in the triggering request; Length =  0x00; Simple Descriptor field absent.
3c	DUT unicasts Simple_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = INV_REQUESTTYPE, NWKAddrOfInterest field set to the value of the same field in the triggering request; Length =  0x00; Simple Descriptor field absent.
4	DUT sends Simple_Desc_rsp with Status NOT_ACTIVE:
DUT unicasts Simple_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = NOT_ACTIVE, NWKAddrOfInterest field set to the short address of the DUT; Length =  0x00; Simple Descriptor field absent.
6	DUT sends Simple_Desc_rsp with Status INVALID_EP:
DUT unicasts Simple_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8004, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = INVALID_EP, NWKAddrOfInterest field set to the short address of the DUT; Length =  0x00; Simple Descriptor field absent.
9a	DUT responds with empty Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp with Status = SUCCESS, NwkAddrOfInterest set to the short address of the DUT, MatchLength = 0x00 and the MatchList absent.
9b	DUT does NOT send Match_Desc_rsp.
10a	DUT responds with empty Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp with Status = SUCCESS, NwkAddrOfInterest set to the short address of the DUT, MatchLength = 0x00 and the MatchList absent.
10b	DUT does NOT send Match_Desc_rsp.
11a	THr1 unicasts to the short address of the DUT a Match_Desc_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00 APS Counter field present, ZDP Transaction Sequence Number present, NWKAddrOfInterest field set to a short address NOT present in the network, ProfileID = 0x0104 (common), and the cluster lists including at least one cluster implemented by the DUT.
11b	DUT unicasts Match_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = DEVICE_NOT_FOUND, NWKAddrOfInterest field set to the value of the same field in the triggering request; MatchLength =  0x00; MatchList field absent.
11c	DUT unicasts Match_Desc_rsp command to the network address of the THr1, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x8006, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and having any value, ZDP Transaction Sequence Number present and having the same value as the triggering request, Status = INV_REQUESTTYPE, NWKAddrOfInterest field set to the value of the same field in the triggering request; MatchLength =  0x00; MatchList field absent.
12a	DUT responds with empty Match_Desc_rsp with status SUCCESS:
DUT unicasts to the THr1 a correctly formatted Match_Desc_rsp with Status = SUCCESS, NwkAddrOfInterest set to the short address of the DUT, MatchLength = 0x00 and the MatchList absent.
12b	DUT does NOT send Match_Desc_rsp.

Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
