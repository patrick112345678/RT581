11.13 TP/APS/BV-13 ZC/ZR- APS TX Multiple Data Frame
Objective: Verify that ZC and ZR pend data to multiple addresses (ZED)

      gZC
      |
    DUT ZR1
|           |
gZED1       gZED2

gZC: PANid= 0x1AAA Logical Address = 0x0000
ZR DUT: PANid= 0x1AAA; Test set-up may be such that DUT is ZC itself (0x0000);

Set ZC under target stack profile, ZC as coordinator starts a PAN = 0x1AAA network ZR joins gZED1, gZED2; gZC is parent of ZR
gZED1 and gZED2 shall be polling its parent once every two seconds for synchronization.

Test procedure:
1. Broadcast data from gZC to ZED1, ZED2 ( via ZR DUT)
Send 3 counted data packets, 2 every seconds
DstEndpoint=0xf0
ProfileId=0x7f01=Test profileID
ClusterID=0x0001=Test clusterID
SrcEndpoint=0x01=endpoint 1 on gZC
asduLength=0xA
asdu=data packet
TxOptions=0x04
DiscoverRoute=0x00
RadiusCounter=0x07

2. gZED1, gZED2 polls for data to ZR according to its polling
policy.


Pass verdict:
1) gZC broadcasts over-the-air APS Data frame 3 times, 1 every 2 seconds.
DstEndpoint=0xf0
ProfileId=0x7f01=Test profileID
ClusterID=0x0001=Test clusterID
SrcEndpoint=0x01=endpoint 1 on gZC
asduLength=0x0a
asdu=counted packets

2) DUT ZR unicasts in indirect transmission (pending frame) over-the-air APS
Data frame
DstAddrMode=0x02=16-bit
DstAddr=0xffff
DstEndpoint=0xf0
ProfileId=0x7f01=Test profileID
ClusterID=0x0001=Test clusterID
SrcEndpoint=0x01=endpoint 1 on gZC
asduLength=0x0A
asdu=counted packets
DUT ZR unicasts in direct transmission (no pending frame) over-the-air APS
DstAddrMode=0x02=16-bit
DstAddr=0xffff
DstEndpoint=0xf0
ProfileId=0x7f01=Test profileID
ClusterID=0x1=Test clusterID
SrcEndpoint=0x01=endpoint 1 on gZC
asduLength=0x0A
asdu=counted packets
Note: Beacon from ZR, based on beacon_req. shows pending data for gZED1 only.

3) ZR unicasts to gZED1 and gZED2 upon synchronization/data request from
gZED1 (MAC data_request). Note for a non-beacon PAN, data is transferred upon
polling by gZED1. and gZED2.

Fail verdict:
1) gZC does not broadcast the APS Data frame
2) ZR does not unicast the APS Data frame directly or in pending form.
3) Upon request from gZED1 and/or gZED2, ZR does not transmit the APS Data frame originally from gZC.
