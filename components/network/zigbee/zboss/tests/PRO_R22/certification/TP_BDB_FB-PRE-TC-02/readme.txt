5.1.3 B-PRE-TC-02: Device discovery – server side tests
This test verifies the reception of device discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the device discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the device discovery request primitives mandated by the BDB specification.
Additional optional devices

THe1 - TH Sleeping ZED
This device is only required if the DUT is a ZC/ZR, to test the child address caching
This role can be performed by a golden unit or a test harness


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P1 - DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE).
If DUT is a ZED, it shall join at the THr1, to prevent that discovery information caching by the parent is involved.
P2 - THe1 is factory new and off.

Test procedure:
Replace step 1&2 of TP/ZDO/BV-09 with: Reception of NWK_addr_req command
1	Conditional on DUT being a ZR/ZC: Broadcast NWK_addr_req, extended response:
THr1 broadcasts to 0xfffd the NWK_addr_req command, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x0000, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, with the command payload fields set as follows: 
IEEEAddress field set to the IEEE address of the DUT, 
Request Type set to 0x01 (extended response), StartIndex = 0x00.	DUT sends a NWK_addr_rsp with Status = SUCCESS:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with:
APS header:
Frame Control sub-fields: Frame Type = 0b00, Ack. Request = 0b1;  
Destination Endpoint = 0x00,
ClusterID=0x8000, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
NumAssocDev =0x00 
StartIndex and NWKAddrAssocDevList NOT present.

2	Conditional on DUT being ZR/ZC: Make a THe1 join at DUT.	

3	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Broadcast NWK_addr_req, single device:
THr1 broadcasts to 0xfffd the NWK_addr_req command, correctly protected with the network key, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x0000, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, 
with the command payload fields set as follows: 
IEEEAddress field set to the IEEE address of the DUT, 
Request Type set to 0x00 (single device), StartIndex = 0x00.	DUT responds with NWK_addr_rsp with the status SUCCESS.
DUT unicasts to the THr1 a correctly protected NWK_addr_rsp, with:
APS header: 
Frame control sub-fields: Frame Type = 0b00, Ack. Request = 0b1;  
Destination Endpoint = 0x00, 
ClusterID=0x8000, ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

4a	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Broadcast NWK_addr_req, extended response:
THr1 broadcasts to 0xfffd the NWK_addr_req command for the DUT.	THr1 broadcasts to 0xfffd the NWK_addr_req command, correctly protected with the network key, with:
APS header:
 Frame Control sub-fields: Frame Type = 0b00, 
Destination Endpoint = 0x00, 
ClusterID=0x0000, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, 
with the command payload fields set as follows: 
IEEEAddress field set to the IEEE address of the DUT, 
Request Type set to 0x01 (extended response), StartIndex = 0x00.

4b	Conditional on 4a and DUT being a ZR/ZC with associated ZED devices:
DUT responds with NWK_addr_rsp with the status SUCCESS.	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.

4c	Conditional on 4a and DUT being THe1 with RxOnWhenIdle = TRUE: DUT responds with NWK_addr_rsp with the status SUCCESS. 	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

5a	Unicast NWK_addr_req, extended response:
THr1 unicasts to the short address of the DUT the NWK_addr_req command for the DUT.	THr1 unicasts to the short address of the DUT a correctly formatted NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT, Request Type set to 0x01 (extended response), StartIndex = 0x00.

5b	Conditional on DUT being a ZR/ZC with associated ZED devices:
DUT responds with NWK_addr_rsp with the status SUCCESS.	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.

5c	Conditional on DUT being THe1 with RxOnWhenIdle = TRUE: 
DUT responds with NWK_addr_rsp with the status SUCCESS.	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

6	Conditional on DUT being ZR/ZC with associated ZED device: Unicast NWK_addr_req for a child ZED node:
THr1 unicasts to the short address of the DUT a correctly formatted  NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT’s child, Request Type set to 0x00 (single device response).	DUT responds with NWK_addr_rsp with the status SUCCESS
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT’s child; 
NWKAddrRemoteDev = network address of the DUT’s child; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

7	Conditional on DUT being ZR/ZC with associated device: Broadcast extended NWK_addr_req for a child ZED node:
THr1 broadcasts to 0xfffd a correctly formatted NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT’s ZED child, Request Type set to 0x01 (extended response), StartIndex = 0x00.	DUT responds with NWK_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
NWKAddrAssocDevList including the DUT’s ZED children, and the fields NumAssocDev, StartIndex set accordingly.

8A	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: broadcast with incorrect request type:
THr1 broadcasts to 0xfffd a correctly formatted NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT, Request Type set to 0x02 (reserved), StartIndex = 0x00.	Since the request is a broadcast, DUT does NOT respond with the NWK_addr_rsp command.

8B	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: unicast with incorrect request type:
THr1 unicasts to the DUT a correctly formatted NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT, Request Type set to 0x02 (reserved), StartIndex = 0x00.	Since the request is a unicast, DUT responds with the NWK_addr_rsp command, with the status INV_REQUESTTYPE:
DUT unicasts to THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = INV_REQUESTTYPE;
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

9	Negative test: unicast request for unknown IEEE address:
THr1 unicasts to the DUT a correctly formatted  NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address NOT present in the network, Request Type set to 0x00 (single device).	DUT responds with the NWK_addr_rsp command, with the status DEVICE_NOT_FOUND:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = DEVICE_NOT_FOUND; 
IEEEAddrRemoteDev = IEEE address from the request; 
NWKAddrRemoteDev = 0xffff; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

10	Conditional on DUT being a ZR/ZC or a ZED with macRxOnWhenIdle = TRUE: Negative test: broadcast request for unknown IEEE address:
THr1 broadcasts to 0xfffd a correctly formatted  NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address NOT present in the network, Request Type set to 0x00 (single device).	DUT does NOT send a NWK_addr_rsp.

11	Conditional on DUT being ZR/ZC:
Make THe1 leave the network.	
Replace step 1&2 of TP/ZDO/BV-09 with: Reception of NWK_addr_req command

12	Conditional on DUT being a ZR/ZC with NO associated ZED devices: Unicast to DUT the IEEE_addr_req, extended response:
THr1 unicasts to the DUT the IEEE_addr_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, with the command payload fields set as follows: NwkAddrOfInterest field set to the NWK address of the DUT, Request Type set to 0x01 (extended response), StartIndex = 0x00.	DUT sends an IEEE_addr_rsp with Status = SUCCESS:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev =0x00, StartIndex and NWKAddrAssocDevList NOT present.

13	Conditional on DUT being ZR/ZC: Make a THe1 join at DUT.	

14	Unicast IEEE_addr_req, single device:
THr1 unicasts to the DUT the IEEE_addr_req command, correctly protected with the network key, with Frame Type sub-field of the APS header Frame Control field set to 0b00, Destination Endpoint = 0x00, ClusterID=0x0001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present, ZDP Transaction Sequence Number present, with the command payload fields set as follows: NWKAddrOfInterest field set to the NWK address of the DUT, Request Type set to 0x00 (single device), StartIndex having any value.	DUT responds with IEEE_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly protected IEEE_addr_rsp, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

15a	Unicast IEEE_addr_req, extended response:
THr1 unicasts to the DUT the IEEE_addr_req command for the DUT.	THr1 unicasts to the DUT a correctly formatted IEEE_addr_req command, with the command payload fields set as follows: NwkAddrOfInterest field set to the NWK address of the DUT, Request Type set to 0x01 (extended response), StartIndex = 0x00.

15b	Conditional on 15a and DUT being a ZR/ZC with associated ZED devices:
DUT responds with IEEE_addr_rsp with the status SUCCESS.	DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
 NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.

15c	Conditional on 15a and DUT being a THe1: DUT responds with IEEE_addr_rsp with the status SUCCESS. 	DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

16	Conditional on DUT being ZR/ZC with associated ZED device: Unicast IEEE_addr_req for a child ZED node:
THr1 unicasts to the short address of the DUT a correctly formatted  IEEE_addr_req command, with the command payload fields set as follows: NWKAddrOfInterest field set to the NWK address of the DUT’s child, Request Type set to 0x00 (single device response).	DUT responds with IEEE_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT’s child; 
NWKAddrRemoteDev = network address of the DUT’s child; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

17	Negative test: incorrect request type:
THr1 unicasts to the DUT a correctly formatted  IEEE_addr_req command, with the command payload fields set as follows: NWKAddrOfInterest field set to the NWK address of the DUT, Request Type set to 0x02 (reserved), StartIndex = 0x00.	DUT responds with the IEEE_addr_rsp command, with the status INV_REQUESTTYPE:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = INV_REQUESTTYPE; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

18	Negative test: unicast request for unknown IEEE address:
THr1 unicasts to the DUT a correctly formatted  IEEE_addr_req command, with the command payload fields set as follows: NWKAddrOfInterest field set to the NWK address NOT present in the network, Request Type set to 0x00 (single device), StartIndex having any value.	DUT responds with the IEEE_addr_rsp command, with the status DEVICE_NOT_FOUND:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = DEVICE_NOT_FOUND; 
IEEEAddrRemoteDev = 0xffffffffffffffff; 
NWKAddrRemoteDev = network address from the request; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

Verification:
1	DUT sends a NWK_addr_rsp with Status = SUCCESS:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with:
APS header:
Frame Control sub-fields: Frame Type = 0b00, Ack. Request = 0b1;  
Destination Endpoint = 0x00,
ClusterID=0x8000, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
NumAssocDev =0x00 
StartIndex and NWKAddrAssocDevList NOT present.

3	DUT responds with NWK_addr_rsp with the status SUCCESS.
DUT unicasts to the THr1 a correctly protected NWK_addr_rsp, with:
APS header: 
Frame control sub-fields: Frame Type = 0b00, Ack. Request = 0b1;  
Destination Endpoint = 0x00, 
ClusterID=0x8000, ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

4a	THr1 broadcasts to 0xfffd the NWK_addr_req command, correctly protected with the network key, with:
APS header:
 Frame Control sub-fields: Frame Type = 0b00, 
Destination Endpoint = 0x00, 
ClusterID=0x0000, 
ProfileID = 0x0000 (ZDP), 
Source Endpoint = 0x00, 
APS Counter field present, 
ZDP Transaction Sequence Number present, 
with the command payload fields set as follows: 
IEEEAddress field set to the IEEE address of the DUT, 
Request Type set to 0x01 (extended response), StartIndex = 0x00.

4b	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.

4c	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

5a	THr1 unicasts to the short address of the DUT a correctly formatted NWK_addr_req command, with the command payload fields set as follows: IEEEAddress field set to the IEEE address of the DUT, Request Type set to 0x01 (extended response), StartIndex = 0x00.

5b	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.

5c	DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

6	DUT responds with NWK_addr_rsp with the status SUCCESS
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT’s child; 
NWKAddrRemoteDev = network address of the DUT’s child; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

7	DUT responds with NWK_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network  address of the DUT; 
NWKAddrAssocDevList including the DUT’s ZED children, and the fields NumAssocDev, StartIndex set accordingly.

8A	Since the request is a broadcast, DUT does NOT respond with the NWK_addr_rsp command.

8B	Since the request is a unicast, DUT responds with the NWK_addr_rsp command, with the status INV_REQUESTTYPE:
DUT unicasts to THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = INV_REQUESTTYPE;
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

9	DUT responds with the NWK_addr_rsp command, with the status DEVICE_NOT_FOUND:
DUT unicasts to the THr1 a correctly formatted NWK_addr_rsp, with the command payload fields set as follows: 
Status = DEVICE_NOT_FOUND; 
IEEEAddrRemoteDev = IEEE address from the request; 
NWKAddrRemoteDev = 0xffff; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

10	DUT does NOT send a NWK_addr_rsp.

12	DUT sends an IEEE_addr_rsp with Status = SUCCESS:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, 
APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev =0x00, StartIndex and NWKAddrAssocDevList NOT present.

14	DUT responds with IEEE_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly protected IEEE_addr_rsp, with: 
Frame Type sub-field of the APS header Frame Control field set to 0b00, 
Destination Endpoint = 0x00, ClusterID=0x8001, ProfileID = 0x0000 (ZDP), Source Endpoint = 0x00, APS Counter field present and set to any value, 
ZDP Transaction Sequence Number present and set to the value of ZDP TSN of the triggering request, 
with the command payload fields set as follows: 
Status = SUCCESS; IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

15a	THr1 unicasts to the DUT a correctly formatted IEEE_addr_req command, with the command payload fields set as follows: NwkAddrOfInterest field set to the NWK address of the DUT, Request Type set to 0x01 (extended response), StartIndex = 0x00.

15b	DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
NumAssocDev corresponding to the number of ZED devices included in the NWKAddrAssocDevList, 
StartIndex = 0x00, 
 NWKAddrAssocDevList listing as many short addresses of associated ZED devices as fit the maximum APS packet length; it shall include the THe1.
 
15c	DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

16	DUT responds with IEEE_addr_rsp with the status SUCCESS:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = SUCCESS; 
IEEEAddrRemoteDev = IEEE address of the DUT’s child; 
NWKAddrRemoteDev = network address of the DUT’s child; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

17	DUT responds with the IEEE_addr_rsp command, with the status INV_REQUESTTYPE:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = INV_REQUESTTYPE; 
IEEEAddrRemoteDev = IEEE address of the DUT; 
NWKAddrRemoteDev = network address of the DUT; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.

18	DUT responds with the IEEE_addr_rsp command, with the status DEVICE_NOT_FOUND:
DUT unicasts to the THr1 a correctly formatted IEEE_addr_rsp, with the command payload fields set as follows: 
Status = DEVICE_NOT_FOUND; 
IEEEAddrRemoteDev = 0xffffffffffffffff; 
NWKAddrRemoteDev = network address from the request; 
the fields NumAssocDev, StartIndex, and NWKAddrAssocDevList are NOT included.


Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zr, zc or zed
 
 
