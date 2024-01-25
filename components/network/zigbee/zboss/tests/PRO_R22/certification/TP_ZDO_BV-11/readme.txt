11.11 TP/ZDO/BV-11: ZED-ZDO-Transmit Service Discovery 
The DUT as ZigBee end device shall request service discovery to a ZigBee coordinator.

gZC	PANid= 0x1AAA
	Logical Address = 0x0000
	0xaa aa aa aa aa aa aa aa
ZED1 	PANid= 0x1AAA
	0x00 00 00 00 00 00 00 01
ZED2	PANid= 0x1AAA
	0x00 00 00 00 00 00 00 02

Initial condition:
1. Reset DUT
2. Set gZC under target stack profile, gZC as coordinator starts a PAN = 0x1AAA network
Set NodeDescriptor values as:
Logical type=0b000, Reserved=0b000, APS flags=0b000, Frequency=0b01000, MAC capability flags=0b0X001X11, Manufacturer code=0x00, Maximum buffer size (unit dependent) , Maximum transfer size=0x0)
Set PowerDescriptor values as:
Current power mode=0b0000, Available power mode=0b0111, Current power source=0b0011, Current power source level=0b1100): Note that PowerDescriptor values may be set to platform specific values.
Test Profile Test Driver Device Id=0x0000 on gZC endpoint=0x01
Set SimpleDescriptor values as:
Endpoint=0x01, Application profile identifier=0x0103, Application device identifier=0x0000, Application device version=0b0000, Application flags=0b0000,  Application input cluster count=0x0A, Application input cluster list=0x00 0x03 0x04 0x38 0x54 0x70 0x8c 0xc4 0xe0 0xff , Application output cluster count=0x0A, Application output cluster list=0x00 0x01 0x02 0x1c 0x38 0x70 0x8c 0xa8 0xc4 0xff)
Set ComplexDescriptor values as:
	<languageChar>:AAAA
	<manufacturerName>:BBBB
	<modelName>:CCCC
	<serialNumber>:1234
	<deviceURL>:255.255.255.255
	<icon>:DDDD
	<iconURL>:0.0.0.0
	reserved
Set UserDescriptor=0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
3. DUT ZED1 joins the PAN through ZDO network manager functionality
4. gZED2 joins the PAN through ZDO network manager functionality

Test procedure:
1. DUT ZED1 ZDO issues Node_Desc_req to parent ZDO
NWKAddrOfInterest=0x0000=gZC 16-bit short NWK address
2. DUT ZED1 ZDO issues Power_Desc_req to the gZC.
NWKAddrOfInterest=gZC 16-bit NWK address
3. DUT ZED1 ZDO issues Simple_Desc_req to the gZC.
NWKAddrOfInterest=0x0000 16-bit gZC NWK address
Endpoint=endpoint number of test profile in gZC
4. DUT ZED1 ZDO issues Active_EP_req to the gZC.
NWKAddrOfInterest=0x0000 16-bit gZC NWK address
5. DUT ZED1 ZDO issues Match_Descr_req to the gZC.
NWKAddrOfInterest=0x0000 16-bit gZC NWK address
ProfileID=Profile of interest to match
NumInClusters=Number of input clusters to match=0x02, InClusterList=matching cluster list=0x54 0xe0
NumOutClusters=return value=0x03
OutClusterList=return value=0x1c 0x38 0xa8
6. DUT ZED1 ZDO issues Complex_Descr_req to the gZC.
NWKAddrOfInterest=0x0000=gZC 16-bit NWK address
7. DUT ZED1 ZDO issues User_Desc_req to the gZC
NWKAddrOfInterest=0x0000=gZC 16-bit NWK address
8. DUT ZED1 ZDO issues Discovery_Register_req to the gZC.
NWKAddrOfInterest=0x796F 16-bit DUT ZED1 NWK address
IEEE address=64-bit DUT ZED1 IEEE address=0x0000000000000001
9. DUT ZED1 ZDO issues User_Desc_Set to the gZC.
NWKAddrOfInterest=0x0000 16-bit gZC NWK address UserDescription=”Dummy Text”
10. DUT ZED1 ZDO issues User_Desc_req to the gZC. NWKAddrOfInterest=0x0000 16-bit gZC NWK address
11. DUT ZED1 ZDO issues End_Device_annce to the gZC. NWKAddrOfInterest=0x796F 16-bit DUT ZED1 NWK address
IEEE address = 64-bit IEEE Address of DUT ZED1=0x0000000000000001

Pass verdict:
1) DUT ZED1 transmits a unicast transmission over the air with APS data frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0002
SrcEndpoint=0x00
asduLength=0x1c
asdu=Node_Desc_req(NWKAddrOfInterest=0x0000=gZC 16-bit short NWK address)
2) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=ZED1=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8002
SrcEndpoint=0x00
asduLength
asdu=Node_Desc_rsp(Status=0x00, NWKAddrOfInterest=0x0000=gZC 16-bit short NWK address, NodeDescriptor= Logical type=0b000, Reserved=0b000, APS flags=0b000, Frequency=0b01000, MAC capability flags=0b0X001X11, Manufacturer code=0x00, Maximum buffer size (unit dependent)  , Maximum transfer size=0x0) 
3) DUT ZED1 transmits a unicast transmission over the air with APS data frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x08003
SrcEndpoint=0x00
asduLength=0x0c
asdu=Power_Desc_Addr_req(NWKAddrOfInterest=0x0000 gZC 16-bit NWK address)
4) gZC transmits an APS data frame MSG	
DstAddrMode=0x02=16-bit
DstAddr=ZED1
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8003
SrcEndpoint=0x00
asduLength
asdu=Power_Desc_Addr_rsp (Status=0x00=SUCCESS, NWKAddrOfInterest=0x0000, IEEE 64 bit address of gZC, PowerDescriptor=Current power mode=0b0000, Available power mode=0b0111, Current power source=0b0011, Current power source level=0b1100): Note that PowerDescriptor values may be set to  platform specific values.
5) DUT ZED1 transmits a unicast transmission over the air with APS data 	frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0004
SrcEndpoint=0x00
asduLength=0x09
asdu=Simple_Desc_req(NWKAddrOfInterest=0x0000 16-bit gZC NWK address
Endpoint=0x01=endpoint number of test profile in gZC)
6) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=ZED1=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x00
ClusterID=0x8004
SrcEndpoint=0x00
asduLength
asdu=Simple_Desc_rsp(Status=0x00=SUCCESS, NWKAddrOfInterest=0x0000 16-bit gZC NWK address, length, SimpleDescriptor=
Endpoint=0x01, Application profile identifier=0x0103, Application device 	identifier=0x0000, Application device version=0b0000, Application 	flags=0b0000,  Application input cluster count=0x0A, Application input 	cluster 	list=0x00 0x03 0x04 0x38 0x54 0x70 0x8c 0xc4 0xe0 0xff, 	Application output cluster count=0x0A, Application output cluster 	list=0x00 0x01 0x02 0x1c 0x38 0x70 0x8c 0xa8 0xc4 0xff)
7) DUT ZED1 transmits a unicast transmission over the air with APS data 	frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x00
ClusterID=0x0005
SrcEndpoint=0x00
asduLength=0x08
asdu=Active_EP_req(NWKAddrOfInterest=0x0000 16-bit gZC NWK address)
8) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=ZED1=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8005
SrcEndpoint=0x00
asduLength
asdu=Active_EP_rsp(Status=0x00, NWKAddrOfInterest=0x0000, ActiveEPCount=0x01, ActiveEPList= 0x01)
9) DUT ZED1 transmits a unicast transmission over the air with APS data 	frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0006
SrcEndpoint=0x00
asduLength
asdu=Match_Descr_req(NWKAddrOfInterest=0x0000 16-bit gZC NWK address, ProfileID=Profile of interest to match=0x0C20, NumInClusters=Number of input clusters to match=0x02, InClusterList=matching cluster list=0x54 0xe0, NumOutClusters=return value=0x03, OutClusterList=return value=0x1c 0x38 0xa8)
10) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x00
ClusterID=0x8006
SrcEndpoint=0x00
asduLength
asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, MatchLength=0x01, MatchList=0x01)
11) DUT ZED1 transmits a unicast transmission over the air with APS data 			frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0010
SrcEndpoint=0x00
asduLength
asdu=Complex_Desc_req(NWKAddrOfInterest=0x0000=gZC 16-bit NWK address)
12) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8010
SrcEndpoint=0x00
asduLength
asdu=Complex_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, 	ComplexDescriptor:
	reserved
	<languageChar>:AAAA
	<manufacturerName>:BBBB
	<modelName>:CCCC
	<serialNumber>:1234
	<deviceURL>:255.255.255.255
	<icon>:DDDD
	<iconURL>:0.0.0.0
	reserved
OR
asdu=Complex_Desc_rsp(Status=0x01=Not Supported, NWKAddrOfInterest=0x0000, Length, ComplexDescriptor: NULL)
13) DUT ZED1 transmits a unicast transmission over the air with APS data 			frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0011
SrcEndpoint=0x00
asduLength
asdu=User_Desc_req(NWKAddrOfInterest=0x0000=gZC 16-bit NWK address)
14) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=ZED1=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x00
ClusterID=0x8011
SrcEndpoint=0x00
asduLength
asdu=User_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, UserDescriptor=0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
OR
Status=0x01=NOT_SUPPORTED, NWKAddrOfInterest=0x0000, Length, 	UserDescriptor=0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88)
15) DUT ZED1 transmits a unicast transmission over the air with APS data 			frame:
DstAddrMode=0x02=16-bit
DstAddr=ZC=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0012
SrcEndpoint=0x00
asduLength
asdu=Discovery_Register_req(NWKAddrOfInterest=0x796F 16-bit DUT ZED1 NWK address
IEEE address=64-bit DUT ZED1 IEEE address=0x0000000000000001)
16) gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=ZC=ZED1=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8012
SrcEndpoint=0x0
asduLength
asdu=Discovery_Register_rsp(Status=0x01=NOT_SUPPORTED, 					NWKAddrOfInterest=NULL)
17) ZED1 transmits a unicast transmission over the air with APS data 	frame:
DstAddrMode=0x02=16-bit
DstAddr=ZC=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0014
SrcEndpoint=0x00
asduLength
asdu=User_Desc_Set_req(NWKAddrOfInterest=0x796F 16-bit ZED1 NWK address
UserDescription=”Dummy Text”, IEEE address=64-bit ZED1 IEEE address=0x0000000000000001)
18) The DUT gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=gZC=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0014
SrcEndpoint=0x00
asduLength
asdu=User_Desc_Conf(Status=0x00=SUCCESS) OR User_Desc_Conf(Status=0x84=Not_Supported, NWKAddrOfInterest=0x0000)
19) DUT ZED1 transmits a unicast transmission over the air with APS data 	frame:
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0011
SrcEndpoint=0x00
asduLength
asdu=User_Desc_req(NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address)
20) The gZC transmits an APS data frame MSG
DstAddrMode=0x02=16-bit
DstAddr=0x0000=unicast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x8011
SrcEndpoint=0x00
asduLength
asdu=User_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, UserDescriptor=’Dummy Text’
OR
Status=0x01=NOT_SUPPORTED, NWKAddrOfInterest=0x0000)
21) DUT ZED1 transmits a unicast transmission over the air with APS data 			frame:
DstAddrMode=0x02=16-bit
DstAddr=0xffff=broadcast
DstEndpoint=0x0=ZDO
ProfileId=0x0000
ClusterID=0x0013
SrcEndpoint=0x00
asduLength
asdu=End_Device_annce message(NWKAddrOfInterest=0x796F 16-bit DUT ZED1 NWK address, IEEE address = 64-bit IEEE Address of DUT ZED1=0x0000000000000001)


Fail verdict:
    1) DUT ZED1 over-the-air packet is not correct
    2) gZC Condition 
    3) DUT ZED1 over-the-air packet is not correct
    4) gZC Condition 
    5) DUT ZED1 over-the-air packet is not correct
    6) gZC Condition 
    7) DUT ZED1 over-the-air packet is not correct
    8) gZC Condition 
    9) DUT ZED1 over-the-air packet is not correct
    10) gZC Condition 
    11) DUT ZED1 over-the-air packet is not correct
    12) gZC Condition 
    13) DUT ZED1 over-the-air packet is not correct
    14) gZC Condition 
    15) DUT ZED1 over-the-air packet is not correct
    16) gZC Condition 
    17) DUT ZED1 over-the-air packet is not correct
    18) gZC Condition 
    19) DUT ZED1 over-the-air packet is not correct
    20) gZC Condition 
    21) DUT ZED1 over-the-air packet is not correct

Comments:
- DUT ZED1 ZDO issues Node_Desc_req to parent ZDO
- DUT ZED1 ZDO issues Power_Desc_req to the gZC
- DUT ZED1 ZDO issues Simple_Desc_req to the gZC
- DUT ZED1 ZDO issues Active_EP_req to the gZC
- DUT ZED1 ZDO issues Match_Descr_req to the gZC
- DUT ZED1 ZDO issues Complex_Descr_req to the gZC - NOT supported
- DUT ZED1 ZDO issues User_Desc_req to the gZC - NOT support
- DUT ZED1 ZDO issues Discovery_Register_req to the gZC - NOT supported
- DUT ZED1 ZDO issues User_Desc_Set to the gZC - NOT supported
- DUT ZED1 ZDO issues User_Desc_req to the gZC - NOT supported
- DUT ZED1 ZDO issues End_Device_annce to the gZC


To execute test start runng.sh and check logs for result
