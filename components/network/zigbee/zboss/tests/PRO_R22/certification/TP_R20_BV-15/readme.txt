TP/R20/BV-15: Messages with Wildcart Profile (CCB 1224)
(CCB 1224)
Objective: To confirm that a device will answer a Match Descriptor request and other APS data messages using the wildcard profile ID (0xFFFF).
DUT: ZR1


The required network setup for the test is as follows:
gZC:
   EPID = 0x0000 0000 0000 0001
   PAN ID = 0x1aaa
   Short Address = 0x0000
   Long Address = 0x AAAA AAAA AAAA AAAA

DUT ZR1:
   EPID = 0x0000 0000 0000 0001
   PAN ID = 0x1aaa
   Short Address = Generated in a random manner within the range 1 to 0xFFF7
   Long Address = 0x 0000 0001 0000 0000

Test Procedure:
1 Join device DUT ZR1 to gZC.
2 gZC issues a Match_Desc_Req to DUT ZR1
   NWKAddrOfInterest = (address of DUT ZR1)
   ProfileID = 0x7f01 (Test Profile)
   NumInClusters = 0x02
   InClusterList = 0x0054, 0x00e0
   NumOutClusters = 0x03
   OutClusterList = 0x001c, 0x0038, 0x00a8
3 gZC issues a Match_Desc_Req to DUT ZR1
   NWKAddrOfInterest = (address of DUT ZR1)
   ProfileID = 0xFFFF (Wildcard Profile)
   NumInClusters = 0x02
   InClusterList = 0x0054, 0x00e0
   NumOutClusters = 0x03
   OutClusterList = 0x001c, 0x0038, 0x00a8
4 gZC transmits a unicast buffer test request to DUT ZR1 with profile ID 0xFFFF.


Expected Outcome (PASS verdict):
1. DUT ZR1 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2. DUT ZR1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3. DUT ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
4. gZC transmits a unicast APS data message to DUT ZR1.
    ProfileId = 0x0000
    ClusterId = 0x0006
    SrcEndpoint = 0x00
    DstEndpoint = 0x00
    ZDO Match Descriptor Request.
5. DUT ZR1 transmits a unicast APS data message to gZC.
    ProfileID = 0x0000
    ClusterID = 0x8006
    SrcEndpoint = 0x00
    DstEndpoint = 0x00
    ZDO Match descriptor response.
    Status = 0x00 (success)
    NWKAddrOfInterest = (address of DUT ZR1)
    MatchLength = 0x01
    MatchList = 0x01
6. gZC transmits a unicast APS data message to DUT ZR1.
    ProfileId = 0x0000
    ClusterId = 0x0006
    SrcEndpoint = 0x00
    DstEndpoint = 0x00
    ZDO Match Descriptor Request.
7. DUT ZR1 transmits a unicast APS data message to gZC.
    ProfileID = 0x0000
    ClusterID = 0x8006
    SrcEndpoint = 0x00
    DstEndpoint = 0x00
    ZDO Match descriptor response.
    Status = 0x00 (success)
    NWKAddrOfInterest = (address of ZR1)
    MatchLength = 0x01
    MatchList = 0x01
8. gZC transmits a unicast APS data message to DUT ZR1.
    Profile ID = 0xFFFF
    Cluster ID = 0x001C (Buffer test Request)
    SrcEndpoint = 1
    DstEndpoint = 0xF0
9.  DUT ZR1 transmits a unicast APS data message to gZC
     Profile ID = 0x7f01
     Cluster ID = 0x0054 (Buffer Test Response)
     SrcEndpoint = 0xF0
     DstEndpoint = 0x01

Expected Outcome (FAIL verdict):
1. DUT ZR1 does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2. DUT ZR1 is not able to complete the MAC association sequence.
3. DUT ZR1 does not issue a device announcement.
4. gZC does not transmit a ZDO Match descriptor request.
5. DUT ZR1 does not transmit a Match descriptor response.
6. gZC does not transmit a ZDO Match descriptor request.
7. DUT ZR1 does not transmit a Match descriptor response.
8. gZC does not transmit a Buffer Test Request message.
9. DUT ZR1 does not transmit a Buffer Test Response.

Comments: