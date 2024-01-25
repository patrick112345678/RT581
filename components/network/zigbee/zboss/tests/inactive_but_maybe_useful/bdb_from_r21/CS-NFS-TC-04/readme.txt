7.2.4 S-NFS-TC-04: Two devices joining the ZC at the same time; DUT: ZC supporting TC-LK update
There is only one bdbJoiningNodeEui64.
This test covers the behavior of the Zigbee Coordinator with a Trust Centre, when two devices are joining its network at the same time.


Required devices
DUT - ZC hosting the TC role, implementing the BDB
The DUT does NOT require ICs.

THr1 - TH ZR; Zigbee Router joining the centralized network of the DUT
THr2 - TH ZR; Zigbee Router joining the centralized network of the DUT


Initial conditions:
1) A packet sniffer shall be observing the communication over the air interface.
2) For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
   The value of the DUT’s bdbTrustCenterNodeJoinTimeout attribute is known (default = 0xf).
   The value of the DUT’s bdbTrustCenterRequireKeyExchange attribute is FALSE.
   The value of the DUT’s bdbJoinUsesInstallCodeKey attribute is FALSE.


Preparatory steps:
P0a - The DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0 - Network formation is triggered on DUT-ZC.
DUT successfully completes formation of a centralized network.



Test Procedure:
 - Network steering is triggered on the DUT.
 - Power on THr1 and THr2. Network steering is triggered as far as possible simultaneously on THr1 and THr2.
 - Both THr1 and THr2 successfully complete the joining.
 - Within bdbcTCLinkKeyExchangeTimeout THr1 and THr2 both request TC-LK update at the DUT.
 - Conditional on 3a: Within the PermitDuration, DUT sends APS Transport Key to THr1 and THr2, carrying the unique TC-LK.
 - Conditional on 3b:
Within the PermitDuration, both THr1 and THr2 verify correctness of reception of the unique TC-LK.
 - Conditional on 3c:
Within the PermitDuration, DUT responds to both THr1 and THr2 verifying correctness of the unique TC-LK.
 - Note: the test passes if all the TC-LK update and verification steps (1-3d) for both joiners complete, even if some of the steps need to be retried by the joiners, (incl. association).
The test fails if at least one of the joiners does NOT complete all the steps within PermitDuration.

 - Verify correct APS security with two keys
   - Wait for PermitDuration to expire.
   - THr2 unicast to the DUT any APS-protected ZCL/ZDO/APS command requiring response.
   - DUT responds with the matching ZCL/ZDO/APS response
   - THr1 unicast to the DUT any APS-protected ZCL/ZDO/APS command requiring response.
   - DUT responds with the matching ZCL/ZDO/APS response.
