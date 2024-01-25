7.2.3 CS-NFS-TC-03: TC-LK update; DUT: ZC NOT requiring TC-LK update
This test covers the behavior of the Zigbee Coordinator with a Trust Centre, which does NOT require TC-LK update (bdbTrustCenterRequireKeyExchange attribute is FALSE) and does NOT require IC (bdbJoinUsesInstallCodeKey attribute is FALSE).


Required devices:
DUT - ZC hosting the TC role, implementing the BDB
The DUT does NOT require ICs. 
THr1 - TH ZR; Zigbee Router joined to the centralized network of the DUT
THr2 - TH ZR; Zigbee Router joining a centralized network of the DUT


Initial conditions:
1) A packet sniffer shall be observing the communication over the air interface.
2) For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
   The value of the DUT’s bdbTrustCenterNodeJoinTimeout attribute is known (default = 0xf).
   The value of the DUT’s bdbTrustCenterRequireKeyExchange attribute is FALSE.
   The value of the DUT’s bdbJoinUsesInstallCodeKey attribute is FALSE.
3) DUT’s TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT-ZC.


Preparatory steps:
P0a - The DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0 - Network formation is triggered on DUT-ZC.
DUT successfully completes formation of a centralized network.
P1 - THr2 and THr1 are factory new (For information: bdbNodeIsOnANetwork = FALSE) and powered off.


Test Procedure:
 - Test:  Request Key not sent by the THr2
   - Network steering is triggered on the DUT.
   - Power on THr2. Network steering is triggered on the THr2.
   - DUT and THr2 successfully complete the joining.
   - THr2 starts operating on the network.
   - THr2 does NOT send APS Request Key command to the DUT.
   - Wait until  bdbTrustCenterNodeJoinTimeout expires.
   - Conditional on 3a&3b: DUT does NOT remove the THr2 from the network.
   - Conditional on 4: THr2 unicasts to the DUT the Mgmt_Lqi_req.
 
 - Test support of TC-LK update
   - Within the PermitDuration triggered by THr2 joining, Power on THr1. Network steering is triggered on the THr1.
   - Conditional on DUT’s allowRemoteTcPolicyChange =TRUE and DUT’s useWhiteList =FALSE:
DUT and THr1 successfully complete the joining.
   - Within bdbTrustCenterNodeJoinTimeout, THr1 requests TC-LK update at the DUT.
   - Conditional on 7a:
Within bdbcTCLinkKeyExchangeTimeout, DUT sends to THr1 the APS Transport Key carrying the unique TC-LK for THr1.
   - Conditional on 7b:
THr1 correctly verifies the unique TC-LK with the DUT.
 - Verify correct APS security operation with mixed key types
   - Wait for PermitDuration to expire.
   - THr1 unicast to the DUT any APS-protected ZCL/ZDO/APS command requiring response.
   - DUT responds with the matching ZCL/ZDO/APS response
   - THr2 unicast to the DUT any APS-protected ZCL/ZDO/APS command requiring response.
   - DUT responds with the matching ZCL/ZDO/APS response.


Additional info:
DUT's policy:
allowRemoteTcPolicyChange = TRUE and useWhiteList = FALSE
