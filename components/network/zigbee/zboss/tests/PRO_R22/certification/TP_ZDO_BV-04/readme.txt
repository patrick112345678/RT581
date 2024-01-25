11.4 TP/ZDO/BV-04: ZED-ZDO-Transmit Device Discovery Request
The DUT as ZigBee End Device shall request a device discovery to a remote ZigBee coordinator node.

gZC       PANId= 0x1AAA
          Logical Address = 0x0000
          IEEE 0xaa aa aa aa aa aa aa aa
ZED1(DUT) PANId= 0x1AAA
          IEEE 0x00 00 00 00 00 00 00 01
gZED2     PANId= 0x1AAA
          IEEE 0x00 00 00 00 00 00 00 02

Initial condition:
1. Reset DUT as ZC;
2. Set gZC under target stack profile, gZC as coordinator starts a PANId = 0x1AAA network
3. ZED1, DUT joins the PAN
4. gZED2 joins the PAN
where gZC is a tester golden unit, and gZED2 is an additional golden unit.

Test procedure:

1. ZED1, DUT ZDO issues NWK_addr_req to the gZC 
	IEEEAddr=gZC 64-bit IEEE address = 0xaaaaaaaaaaaaaaaa
	RequestType=0x00="single response"
	StartIndex=0x00
	RadiusCounter=0x0a
2.	ZED1, DUT ZDO issues NWK_addr_req to the gZC.
	IEEEAddr=gZC 64-bit IEEE address =0xaaaaaaaaaaaaaaaa
	RequestType=0x01="extended response"
	StartIndex=0x00
3.	ZED1, DUT ZDO issues IEEE_addr_req to the gZC.
	NWKAddrOfInterest=0x0000 16-bit NWK address
	RequestType=0x00="single response"
	StartIndex=0x00
4.	ZED1, DUT ZDO issues IEEE_addr_req to the gZC.
	NWKAddrOfInterest=0x0000 16-bit NWK address
	RequestType=0x01="extended response"
	StartIndex=0x00

Expected Outcome:
1. DUT ZED1 transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=0xffff=broadcast
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x0000
	SrcEndpoint=0x00
	asduLength=0xb
	asdu=NWK_Addr_req(IEEEAddr=gZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, RequestType=0x00="single response", StartIndex=0x00) 
2. gZC transmits over-the-air broadcast APS Data frame:	
	DstAddrMode=0x02=16-bit
	DstAddr=ZED1
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x8000
	SrcEndpoint=0x00
	asduLength=0xb
	asdu=NWK_Addr_rsp(IEEEAddrRemoteDev=gZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev=0x0000, NumAssocDev=0x00, StartIndex=0x00, NWKAddrAssocDevList=NULL)
3. DUT ZED1 transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=0xffff=broadcast
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x0000
	SrcEndpoint=0x00
	asduLength=0x0xb
	asdu=NWK_Addr_req(IEEEAddr=gZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, RequestType=0x01="extended response", StartIndex=0x00)
4. gZC transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=ZED1
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x8000
	SrcEndpoint=0x00
	asduLength=0x12
	asdu=NWK_Addr_rsp(IEEEAddrRemoteDev=gZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev=0x0000, NumAssocDev=0x02, StartIndex=0x00, NWKAddrAssocDevList=gZED1 and gZED2)
5. DUT ZED1 transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=0x0000
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x0001
	SrcEndpoint=0x00
	asduLength=0x5
	asdu=IEEE_addr_req(NWKAddrOfInterest=0x0000, RequestType=0x00="single response", StartIndex=0x00)
6. gZC transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=ZED
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x8001
	SrcEndpoint=0x00
	asduLength=0xc
	asdu=IEEE_addr_rsp(IEEEAddrRemoteDev=64-bit MAC address of itself=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev=0x0000, NumAssocDev=0x00, StartIndex=0x00, NWKAddrAssocDevList=NULL)
7. DUT ZED1 transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=0x0000
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x0001
	SrcEndpoint=0x00
	asduLength=0x5
	asdu=NWK_Addr_req(IEEEAddr=0x0000, RequestType=0x01="extended response", StartIndex=0x00)
8. gZC transmits over-the-air broadcast APS Data frame:
	DstAddrMode=0x02=16-bit
	DstAddr=ZED1
	DstEndpoint=0x00=ZDO
	ProfileId=0x0000
	ClusterID=0x8001
	SrcEndpoint=0x00
	asduLength=0x12
	asdu=NWK_Addr_rsp(IEEEAddrRemoteDev=gZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev=0x0000, NumAssocDev=0x02, StartIndex=0x00, NWKAddrAssocDevList=ZED)

Fail verdict:
	1) gZC over-the-air packet is not correct
	2) ZED1 over-the-air packet is not correct 
	3) gZC over-the-air packet is not correct
	4) ZED1 over-the-air packet is not correct
	5) gZC over-the-air packet is not correct
	6) ZED1 over-the-air packet is not correct
	7) ZED1 over-the-air packet is not correct
	8) gZC over-the-air packet is not correct

11.4.5	Notes
PICS Covered:
A list of PICS that are covered by the Test case. 
PICS Reference	Feature	Tested(Direct Indirect)
AZD1	Mandatory Service and Device Discovery	Direct
AZD2	Mandatory attributes of Service and Device Discovery	Direct
ALF1	Suport of APS-DATA.request,  APS-DATA.confirm	Indirect
ALF2	Support of APS-DATA.indication	Indirect
NLF1	Support of NLDE-DATA.request, NLDE-DATA.confirm	Indirect
NLF2	Support of NLDE-Data.indication	Indirect

