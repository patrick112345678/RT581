Note: test implementation is bit obsolete, test procedure has been changed in r21.

11.9 TP/ZDO/BV-09: ZC-ZDO-Receive Service Discovery
Objective: The DUT as Zigbee coordinator shall respond to mandatory service discovery requests from a remote node.

Initial conditions:

    DUT_ZC
  |        |
  |        |
 gZED1   gZED2

ZC:
   PANid = 0x1AAA
   Logical Address = 0x0000
   0xaa aa aa aa aa aa aa aa
gZED1:
   PANid= 0x1AAA
   0x00 00 00 00 00 00 00 01
gZED2:
   PANid= 0x1AAA
   0x00 00 00 00 00 00 00 02

1 Reset DUT as ZC
2 Load test profile, Test Driver device identifier=0x0000 on ZC endpoint 0x01
3 Set DUT under target stack profile, DUT as coordinator starts a PAN = 0x1AAA network
4 gZED1 joins the PAN through ZDO network manager functionality
5 gZED2 joins the PAN through ZDO network manager functionality
where gZED1, gZED2 are golden units.


Test prcedure:
1 gZED1 ZDO issues Node_Desc_req to parent ZDO NWKAddrOfInterest=DUT=ZC 16-bit IEEE address=0x0000
2 gZED1 ZDO issues Power_Desc_req to the DUT. NWKAddrOfInterest=DUT=ZC 16-bit IEEE address
3 gZED1 ZDO issues Simple_Desc_req to the DUT. NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
  Endpoint=0x01 endpoint number of test profile residing on ZC
  NB: valid endpoints are 1 to 240, and endpoint zero will return invalid
4 gZED1 ZDO issues Active_EP_req to the DUT.
  NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
5 gZED1 ZDO issues Match_Descr_req to the DUT.
  NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
  ProfileID=Profile of interest to match=0x0103
  NumInClusters=Number of input clusters to match=0x02, InClusterList=matching cluster list=0x54 0xe0
  NumOutClusters=return value=0x03
  OutClusterList=return value=0x1c 0x38 0xa8
  NB: test profile currently is limited to small number of clusters
6 gZED1 ZDO issues Match_Descr_req to the DUT. This shall contain a non-matching set of clusters.
  NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
  ProfileID=Profile of interest to match=0x0103
  NumInClusters=Number of input clusters to match=0x02, InClusterList=matching cluster list=0x75
  NumOutClusters=return value=0
  OutClusterList=return value


Pass verdict:
1) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0002
    SrcEndpoint=0x00
    asduLength=0x08
    asdu=Node_Desc_req(NWKAddrOfInterest=DUT=ZC 16-bit address=0x0000)
2) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x8002
    SrcEndpoint=0x00
    asduLength=0x08
    asdu=Node_Desc_rsp(Status=0x00, NWKAddrOfInterest= 0x0000, NodeDescriptor= Logical type=0b000, Reserved=0b000, APS flags=0b000, Frequency=0b01000, MAC capability flags=0b0X001X11, Manufacturer code=0x00, Maximum buffer size (implementation dependent) , Maximum transfer size=0x0)
    Above is parametric information to be verified with ZCP.
3) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0003
    SrcEndpoint=0x0
    asduLength=0x0c
    asdu=Power_Desc_req(NWKAddrOfInterest=DUT=ZC 16-bit address)
    Above is parametric information to be verified with ZCP.
4) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x8003
    SrcEndpoint=0x0
    asduLength=0x0c
    asdu=Power_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, PowerDescriptor=Current power mode=0b0000,
    Available power mode=0b0111, Current power source=0b0001, Current power source level=0b1100)
5) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0004
    SrcEndpoint=0x00
    asduLength=0x09
    asdu=Simple_Desc_req (NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
    Endpoint=0x01 endpoint number of test profile residing on ZC)
6) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0004
    SrcEndpoint=0x00
    asduLength=0x09
    asdu=Simple_Desc_rsp (Status=0x00, NWKAddrOfInterest=0x0000, Length, SimpleDescriptor=
    Endpoint=0x01, Application profile identifier=0x0103, Application device identifier=0x0000,
    Application device version=0b0000, Application flags=0b0000, Application input cluster count=0x0A,
    Application input cluster list=0x00 0x03 0x04 0x38 0x54 0x70 0x8c 0xc4 0xe0 0xff, Application output
    cluster count=0x0A, Application output cluster list=0x00 0x01 0x02 0x1c 0x38 0x70 0x8c 0xa8 0xc4 0xff
7) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0005
    SrcEndpoint=0x00
    asduLength=0x08
    asdu=Active_EP_req(NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address)
8) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x8005
    SrcEndpoint=0x00
    asduLength=0x08
    asdu=Active_EP_rsp(Status=0x00, NWKAddrOfInterest=0x0000, ActiveEPCount=0x01, ActiveEPList= 0x01)
9) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0006
    SrcEndpoint=0x00
    asduLength
    asdu=Match_Descr_req(NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
    ProfileID=Profile of interest to match=0x0103, NumInClusters=Number of input clusters to match=0x02,
    InClusterList=matching cluster list=0x54 0xe0, NumOutClusters=return value=0x03,
    OutClusterList=return value=0x1c 0x38 0xa8)
10) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0006
    SrcEndpoint=0x00
    asduLength
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, MatchLength=0x01, MatchList=0x01)
11) gZED1 transmits a unicast transmission over the air with APS data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0006
    SrcEndpoint=0x00
    asduLength
    asdu=Match_Descr_req(NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
    ProfileID=Profile of interest to match=0x0103, NumInClusters=Number of input clusters to match=1,
    InClusterList=matching cluster list=0x75, NumOutClusters=return value=0, OutClusterList=return value=
12) The DUT ZC transmits an APS data frame MSG:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0006
    SrcEndpoint=0x00
    asduLength
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, MatchLength=0x00, MatchList= <empty>)


Fail verdict:
1) gZED1 over-the-air packet is not correct
2) DUT ZC over-the-air packet is not correct
3) gZED1 over-the-air packet is not correct
4) DUT ZC over-the-air packet is not correct
5) gZED1 over-the-air packet is not correct
7) DUT ZC over-the-air packet is not correct
8) gZED1 over-the-air packet is not correct
9) DUT ZC over-the-air packet is not correct
10) gZED1 over-the-air packet is not correct


Notes: PICS Covered:
A list of PICS that are covered by the Test case.
AZD1 - Mandatory Service and Device Discovery (including server Service Discovery for Node, Power, Simple, Active Endpoint, and Match Descriptors)
AZD2 - Mandatory Service and Device Discovery attributes (including server Service Discovery for Node, Power, Simple, Active Endpoint, and Match Descriptors)
AZD3 - Optional Service and Device Discovery attributes
AZD6 - Optional client Service Discovery Node Descriptor
AZD7 - Optional client Service Discovery Power Descriptor
AZD8 - Optional client Service Discovery Simple Descriptor
AZD9 - Optional client Service Discovery Active Endpoint
AZD10 - Optional client Service Discovery Match Descriptor
AZD17 - Optional client Service Discovery End Device Announce client service
AZD18 - Optional Service Discovery End Device Announce server service


Comments:
