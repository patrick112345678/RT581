11.12 TP/ZDO/BV-12 ZC-ZDO-Transmit Bind/Unbind_req
Verify handling of Bind and Unbind requests.

DUT ZC
gZED1
gZED2

Initial conditions:
1.  Reset DUT, DUT ZC as coordinator starts a PAN = 01AAA network;
2. Set DUT ZC under target stack profile, DUT ZC as coordinator starts a PAN = 0x1AAA network;
3. gZED1 and gZED2 join the PAN via ZDO functionality;
4. gZED2 has been configured such that it has a test profile defined on endpoint=0xF0=240,
and gZED1 with a test profile on endpoint=0x01=1

Test procedure:
1. Perform a BIND on gZED1.
The BIND operation shall be between gZED1 and gZED2.
gZED1 endpoint 0x01 to gZED2 endpoint 0xF0=240 is bound
under the test profile applications running on respective
endpoints, with DeviceId= 0x0000 and 0xaaaa in gZED1 and
gZED2 respectively. Entries consist of gZED1 with 0x001c
output from SrcEdpt=0x1 to DstEdpt=0xF0; gZED1 with
0x0054 output from Srcedpt=0xF0 to DstEdpt=0x1 to gZED2.
2. Test Driver (Device Id 0x0000) on gZED1 issues “Buffer Test Request” (0x001C) with octet sequence length of 0x10 to gZED2.
3. Test Driver (Device Id 0x0000) on gZED2 issues “Buffer Test Request” (0x001C) with octet sequence length of 0x10 to DUT ZED1.
4. The DUT ZC issues UNBIND for gZED1 endpoint=0x01 to gZED2 endpoint=0x0F;
5. Test Driver (Device Id 0x0000) on gZED1 issues “Buffer Test Request” (0x001C) with octet sequence length of 0x10 to gZED2.

Pass verdict:
1) The DUT ZC issues Bind_req to gZED1 for Endpoint=0x01 and gZED2 Endpoint=0xF0 for 0x0001c. 
(verification of source binding at gZED1 can be done by by Mngmt_Bind_req from DUT ZC.
2) gZED1 issues Bind_rsp to DUT ZC with status=Success.
3) gZED1 transmits over-the-air APS Data frame to gZED2 via direct transmission (NWK Dst Addr=gZED2) by way of parent DUT ZC.
    ProfileId=Test profile ID=0x7f01
    ClusterID=0x001c
    SrcEndpoint=0x01=endpoint 1 on DUT ZED1
    asduLength=length of test MSG Buffer_Test_req
    asdu=Buffer_Test_req with sequence length octet=0x10

*response for test buffer
4) gZED2 transmits over-the-air APS Data frame to gZED1 via direct transmission (NWK Dst Addr=DUT ZED1) by way of parent DUT ZC.
    ProfileId=Test profile ID=0x7f01
    ClusterID=0x0054
    SrcEndpoint=0x01=endpoint 1 on DUT ZED1
    asduLength=length of test MSG Buffer_Test_rsp
    asdu=Buffer_Test_rsp with sequence length octet=0x10
5) DUT ZC issues Unbind_req to gZED1, endpoint 0x01=1 as the source, and gZED2, endpoint=0xF0=240 as the destination.
6) gZED1 issues Unbind_rsp with Status=Success.
7) gZED1 does not transmit or returns an error for
    ProfileId=Test profile ID=0x7f01
    ClusterID=0x0001c
    asduLength=length of test MSG Buffer_Test_req
    asdu=Buffer_Test_req with sequence length octet=0x10

Fail verdict:
1) Bind_req is not to gZED1 by DUT ZC.
2) gZED1 Buffer_Test_req is not routed through DUT ZC to gZED2.
3) Unbind_req is not issued to gZED1 by DUT ZC.


Comments:
1) gZED1 and gZED2 join the PAN via ZDO functionality
2) Perform a BIND on gZED1. The BIND operation shall be between gZED1 and gZED2
3) Test Driver (Device Id 0x0000) on gZED1 issues Buffer Test Request (0x001C)
4) The DUT ZC issues UNBIND for gZED1
5) Test Driver (Device Id 0x0000) on gZED1 issues "Buffer Test Request" (0x001C)

To execute test run run.sh. Check log files on test finish.
tp_zdo_bv_06_gZED1*.log should contain "Test is finished, status OK"
message on success.
