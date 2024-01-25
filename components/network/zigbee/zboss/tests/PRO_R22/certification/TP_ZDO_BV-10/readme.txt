11.10 TP/ZDO/BV-10: ZC-ZDO-Receive Service Discovery
The DUT as ZigBee coordinator shall respond to an optional service discovery requests from a remote node.

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
   0x00 00 00 00 00 00 00 01
gZED2:
   0x00 00 00 00 00 00 00 02


1 Reset DUT as ZC
2 Set DUT under target stack profile, DUT as coordinator starts a PAN = 0x1AAA network
  Set ComplexDescriptor:
  reserved
  <languageChar>:AAAA
  <manufacturerName>:BBBB
  <modelName>:CCCC
  <serialNumber>:1234
  <deviceURL>:255.255.255.255
  <icon>:DDDD
  <iconURL>:0.0.0.0
  reserved
  Set UserDescriptor:
  0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
  if Support is "YES"
  ComplexDescriptor Support is "YES"
  UserDescriptor Support is "YES"
3 gZED1 joins the PAN through ZDO network manager functionality
4 gZED2 joins the PAN through ZDO network manager functionality where gZED1, gZED2 are golden units.

Test procedure:
1) gZED1 ZDO issues Complex_Desc_req to parent ZDO NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address
2) gZED1 ZDO issues User_Desc_req to the DUT. NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address
3) gZED1 ZDO issues Discovery_Register_req to the DUT. NWKAddrOfInterest=0x796F 16-bit gZED1 NWK address
   IEEE address=64-bit gZED1 IEEE address=0x0000000000000001
4 gZED1 ZDO issues User_Desc_Set to the DUT.
  NWKAddrOfInterest=0x0000 16-bit ZC NWK address
  UserDescription="Dummy Text"
5 gZED1 ZDO issues User_Desc_req to the DUT.
  NWKAddrOfInterest=0x0000 16-bit ZC
  NWK address

Pass verdict:
1) gZED1 transmits a unicast transmission over the air with APS data frame:
   DstAddrMode=0x02=16-bit
   DstAddr=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x0010
   SrcEndpoint=0x00
   asduLength
   asdu=Complex_Desc_req{ NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address}
2) The DUT ZC transmits an APS data frame MSG
   DstAddrMode=0x02=16-bit
   DstAddr=0x796F=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x8010
   SrcEndpoint=0x00
   asduLength
   asdu=Complex_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, ComplexDescriptor:
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
   asdu=Complex_Desc_rsp(Status=0x01=Not Supported)
3) gZED1 transmits a unicast transmission over the air with APS data frame:
   DstAddrMode=0x02=16-bit
   DstAddr=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x8011
   SrcEndpoint=0x00
   asduLength
   asdu=User_Desc_req(NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address)
4) The DUT ZC transmits an APS data frame MSG
   DstAddrMode=0x02=16-bit
   DstAddr=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x8011
   SrcEndpoint=0x00
   asduLength
   asdu=User_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, UserDescriptor=0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
   OR
   Status=0x01=NOT_SUPPORTED)
5) gZED1 transmits a unicast transmission over the air with APS data frame:
   DstAddrMode=0x02=16-bit
   DstAddr=ZC=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x8012
   SrcEndpoint=0x00
   asduLength=
   asdu=Discovery_Register_req(NWKAddrOfInterest=0x7F6F 16-bit gZED1 NWK address, IEEE address=64-bit gZED1 IEEE address=0x0000000000000001)
6) The DUT ZC transmits an APS data frame MSG
   DstAddrMode=0x02=16-bit
   DstAddr=ZC=0x796F=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x8012
   SrcEndpoint=0x00
   asduLength
   asdu=Discovery_Register_rsp(Status=0x01=NOT_SUPPORTED, NWKAddrOfInterest=0x0000)
7) gZED1 transmits a unicast transmission over the air with APS data frame:
   DstAddrMode=0x02=16-bit
   DstAddr=ZC=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x0014
   SrcEndpoint=0x00
   asduLength
   asdu=User_Desc_Set_req(NWKAddrOfInterest=0x796F 16-bit gZED1 NWK address
   UserDescription=”Dummy Text”, IEEE address=64-bit gZED1 IEEE address=0x0000000000000001)
8) The DUT ZC transmits an APS data frame MSG
   DstAddrMode=0x02=16-bit
   DstAddr=ZC=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x0014
   SrcEndpoint=0x00
   asduLength=
   asdu=User_Desc_Conf(Status=0x00=SUCCESS) OR User_Desc_Conf(Status=0x84=Not_Supported)
9) gZED1 transmits a unicast transmission over the air with APS data frame:
   DstAddrMode=0x02=16-bit
   DstAddr=0x0000=unicast
   DstEndpoint=0x0=ZDO
   ProfileId=0x0000
   ClusterID=0x0011
   SrcEndpoint=0x00
   asduLength
   asdu=User_Desc_req(NWKAddrOfInterest=0x0000=DUT=ZC 16-bit short NWK address)
10) The DUT ZC transmits an APS data frame MSG
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=unicast
    DstEndpoint=0x0=ZDO
    ProfileId=0x0000
    ClusterID=0x8011
    SrcEndpoint=0x00
    asduLength
    asdu=User_Desc_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000, Length, UserDescriptor=’Dummy Text’
    OR
    Status=0x01=NOT_SUPPORTED

Fail verdict:
1) gZED1 Condition
2) DUT ZC over-the-air packet is not correct
3) gZED1 Condition
4) DUT ZC over-the-air packet is not correct
5) gZED1 Condition
7) DUT ZC over-the-air packet is not correct
8) gZED1 Condition
9) DUT ZC over-the-air packet is not correct
10) gZED1 Condition

Notes: PICS Covered:
A list of PICS that are covered by the Test case.
AZD11 (Optional client Service Complex Descriptor) NA
AZD12 (Optional server Service Complex Descriptor) Tested
AZD13 (Optional client Service User Descriptor) NA
AZD14 (Optional server Service User Descriptor) NA
AZD15 (Optional client Service Discovery Discover_Register) Tested
AZD16 (Optional server Service Discovery Discover_Register) Tested
