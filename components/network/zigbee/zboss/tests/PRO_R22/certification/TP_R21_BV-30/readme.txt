
TODO: verify test procedure and implementation!!!

12.30 TP/R21/BV-30 Parent Address Resolution of Children (ZR)
Objective: DUT as ZR answers address requests for its parent children.

gZC:
PANId=0x1AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

gZR:
Router extended address:
0x 00 00 00 01 00 00 00 00

DUT ZED1:
End Device extended address:
0x 00 00 00 00 00 00 00 01

DUT ZED2:
End Device extended address:
0x 00 00 00 00 00 00 00 02

Initial Conditions:
1 All devices have joined using the well-known Trust
Center Link Key (ZigBeeAlliance09), have performed a
Request Key to obtain a new Link Key, and have
confirmed that key with the Verify Key Command.

2 gZED1 and gZED2 both specify an End Device
Timeout of 16 minutes. They have both switched off
polling at the start of the test. Steps 1-6 must be run
within the 16-minute timeout.

3 The value of the NWK key used initially may be any
value, and is known as KEY0 for this test. The Key
Sequence number shall be 0.

         gZC
          |
        DUT_ZR
       |      |
     gZED1  gZED2

Test Procedure:

1 gZC requests the NWK address corresponding to gZED2’s extended address.
2 DUT ZR sends back a response to the NWK_Addr_Req
3 gZC requests the IEEE address corresponding to ZED1’s NWK address.
4 DUT ZR sends back a response to the IEEE Address request.
5 gZC shall broadcast a ZDO NWK Address Request for non-existent device 0xDDDDDDDDDDDDDDDD.
6 DUT ZR does not respond.
7 Allow the End Device Timeout to expire for gZED1 and gZED2.
8 gZC requests the NWK address corresponding to gZED2’s extended address.
9 DUT ZR does not respond.

Pass verdict:
1.	gZC broadcasts a ZDO NWK Address Request with the IEEEAddr field set to the extended address of gZED2, Request Type is set to 0x00 (Single Device Response) and StartIndex is set to 0.  
2.	DUT ZR unicasts a ZDO NWK_Addr_Rsp with the Status of SUCCESS, IEEEAddrRemoteDev set to the extended address of ZED2 and the NWKAddrRemoteDev set to the short address assigned to ZED2.  The NumAssocDev, StartIndex, and NWKAddrAssocDevList fields shall not be present.
3.	gZC broadcasts a ZDO IEEE Address request with the NWKAddrOfInterest field set to the address of ZED1.  RequestType shall be set to Single (0x00), and StartIndex shall be set to 0.
4.	DUT ZR shall unicast a ZDO IEEE_Addr_Rsp to gZC.  The Status field shall be SUCCESS, the IEEEAddrRemoteDev shall be the extended address of ZED1.  The NWKAddrRemoteDev field shall be the assigned short address of ZED1.  The NumAssocDev, StartIndex, and NWKAddrAssocDevList shall not be present.
5.	gZC broadcasts a ZDO NWK Address Request with the IEEEAddr field set to the extended address of 0xDDDDDDDDDDDDDDDD, Request Type is set to 0x00 (Single Device Response) and StartIndex is set to 0.  
6.	DUT ZR shall not send back a ZDO response.
7.	Allow time to pass up to the End Device Timeout specified by gZED1 and gZED2 in the initial conditions.
8.	gZC broadcasts a ZDO NWK Address Request with the IEEEAddr field set to the extended address of gZED2, Request Type is set to 0x00 (Single Device Response) and StartIndex is set to 0.  
9.	DUT ZR shall not send a ZDO message.
