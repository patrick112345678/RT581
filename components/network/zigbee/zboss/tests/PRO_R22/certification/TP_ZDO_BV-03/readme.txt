11.3 TP/ZDO/BV-03: ZC-ZDO-Receive Device Discovery
The DUT as ZigBee coordinator shall respond to a device discovery request from a remote node.

DUT ZC  PANId= 0x1AAA
        Logical Address = 0x0000
        IEEE 0xaa aa aa aa aa aa aa aa
gZED1   PANId= 0x1AAA
        IEEE 0x00 00 00 00 00 00 00 01
gZR     PANId= 0x1AAA
        IEEE 0x00 00 00 00 00 00 00 02

Initial condition:
1. Reset DUT as ZC;
2. Set DUT under target stack profile, DUT as coordinator starts a PANId = 0x1AAA network;
3. gZED1 joins the PAN through ZDO network manager functionality;
4. gZR joins the PAN through ZDO network manager functionality;

Test procedure:
1. gZED1 ZDO issues NWK_addr_req to DUT ZDO for:
    IEEEAddr=DUT=ZC 64-bit IEEE address
    RequestType=0x00=”single response”
    StartIndex=0x00;
2. gZED1 ZDO issues NWK_addr_req to the DUT ZDO:
    IEEEAddr=DUT=ZC 64-bit IEEE address
    RequestType=0x01=”extended response”
    StartIndex=0x00;
3. gZED1 ZDO issues IEEE_addr_req to the DUT.
    NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
    RequestType=0x00=”single response”
    StartIndex=0x00;
4. gZED1 ZDO issues IEEE_addr_req to the DUT.
    NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
    RequestType=0x01=”extended response”
    StartIndex=0x00;

Pass verdict:
1) gZED1 over-the-air broadcast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0xffff=broadcast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0000
    SrcEndpoint=0x00
    asduLength=0xb
    asdu=NWK_addr_req (IEEEAddr =DUT ZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa, RequestType=0x00=”single response”, StartIndex=0x00)
2) DUT ZC over-the-air unicast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x8000
    SrcEndpoint=0x00
    asduLength=0xc
    asdu=NWK_addr_rsp (Status=0x00, IEEEAddrRemoteDev=64-bit IEEE Mac Addr of ZC=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev= 0x0000, NumAssocDev=NULL, StartIndex=NULL, NWKAddrAssocDevList=NULL)
3) gZED1 over-the-air broadcast in APS Data frame:
    DstAddrMode=0x2=16-bit
    DstAddr=0xffff=broadcast
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0000
    SrcEndpoint=0x00
    asduLength=0xb
    asdu=NWK_addr_req (IEEEAddr=DUT ZC 64-bit IEEE address=0xaaaaaaaaaaaaaaaa,
    RequestType=0x01=”extended response”, StartIndex=0x00)
4) DUT ZC over-the-air unicast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1
    DstEndpoint=0x00=ZDO
    ProfileId=0x00
    ClusterID=0x8000
    SrcEndpoint=0x00
    asduLength=0x12
    asdu=NWK_addr_rsp (Status=0x00, IEEEAddrRemoteDev=64-bit IEEE Mac Addr of ZC=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev= 0x0000, NumAssocDev=0x02, StartIndex=0x00, NWKAddrAssocDevList=gZED1)
5) gZED1 over-the-air unicast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x0001
    SrcEndpoint=0x0
    asduLength=0x5
    asdu=IEEE_addr_req(NWKAddrOfInterest=0x0000, RequestType=0x00=”single
    response”,StartIndex=0x00)
6) DUT ZC over-the-air unicast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1
    DstEndpoint=0x00=ZDO
    ProfileId=0x0000
    ClusterID=0x8001
    SrcEndpoint=0x00
    asduLength=0xc
    asdu=IEEE_addr_rsp(Status=0x00, IEEEAddrRemoteDev=64-bit long IEEE MAC address=0xaaaaaaaaaaaaaaaa, NWKAddrRemoteDev = 0x0000, NumAssocDev=NULL, StartIndex=NULL, NWKAddrAssocDevList=NULL)
7 ) gZED1 over-the-air broadcast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000
    DstEndpoint=0x0=ZDO
    ProfileId=0x0000
    ClusterID=0x0001
    SrcEndpoint=0x00
    asduLength=0x5
    asdu=IEEE_addr_req(NWKAddrOfInterest=0x0000, RequestType=0x01=”extended response”, StartIndex=0x00)
8) DUT ZC over-the-air unicast in APS Data frame:
    DstAddrMode=0x02=16-bit
    DstAddr=gZED1
    DstEndpoint=0x0=ZDO
    ProfileId=0x0000
    ClusterID=0x8001
    SrcEndpoint=0x00
    asduLength=0x12
    asdu=IEEE_addr_rsp(Status=0x00, IEEEAddrRemoteDev=64-bit MAC address of itself=0xaaaaaaaaaaaaaaaa , NWKAddrRemoteDev=0x0000, NumAssocDev=0x02, StartIndex=0x00, NWKAddrAssocDevList=gZED1)

Fail verdict:
1) gZED1 condition
2) ZC over-the-air packet is not correct
3) gZED1 condition
4) ZC over-the-air packet is not correct
5) gZED1 condition
6) ZC over-the-air packet is not correct
7) gZED1 condition
8) ZC over-the-air packet is not correct


Notes: PICS Covered:
A list of PICS that are covered by this test case.
AZD1 - Mandatory Service and Device Discovery (Direct)
AZD2 - Mandatory attributes of Service and Device Discovery (Direct)
ALF1 - Support of APS-DATA.request, APS-DATA.confirm (Indirect)
ALF2 - Support of APS-DATA.indication (Indirect)
NLF1 - Support of NLDE-DATA.request, NLDE-DATA.confirm (Indirect)
NLF2 - Support of NLDE-Data.indication (Indirect)

Comments: no


if ( ZG_APS_BIND_DST_TABLE(dst_index).dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG )
  {
    zb_address_short_by_ref(&apsreq_ref->dst_addr.addr_short, ZG_APS_BIND_DST_TABLE(dst_index).u.long_addr.dst_addr);
    if (apsreq_ref->dst_addr.addr_short == ZB_UNKNOWN_SHORT_ADDR)
    {
      /* TODO: we can fall into infinite loop here, asking for peer short address, fix it!!! */
      zb_start_get_peer_short_addr(ZG_APS_BIND_DST_TABLE(dst_index).u.long_addr.dst_addr, zb_nlde_data_confirm, param);
      TRACE_MSG(TRACE_APS3, "-zb_process_transm_from_bindtable: wait for peer addr for buffer %hd", (FMT__H, param));
      return;
    }
    apsreq_ref->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  }