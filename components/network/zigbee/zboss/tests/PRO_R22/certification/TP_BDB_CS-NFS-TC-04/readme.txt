CS-NFS-TC-04: Two devices joining the ZC at the same time; DUT: ZC supporting TC-LK update
There is only one bdbJoiningNodeEui64.
This test covers the behavior of the Zigbee Coordinator with a Trust Centre, when two devices are joining its network at the same time.


Required devices
DUT - ZC hosting the TC role, implementing the BDB
The DUT does NOT require ICs.

THr1 - TH ZR; Zigbee Router joining the centralized network of the DUT
THr2 - TH ZR; Zigbee Router joining the centralized network of the DUT
THe1 - TH ZED; ZigBee End Device joining the centralized network of the DUT at THr1 and THr2

Initial conditions:
1) A packet sniffer shall be observing the communication over the air interface.
2) For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
   The value of the DUT’s bdbTrustCenterNodeJoinTimeout attribute is known (default = 0xf).
   The value of the DUT’s bdbTrustCenterRequireKeyExchange attribute is FALSE.
   The value of the DUT’s bdbJoinUsesInstallCodeKey attribute is FALSE.

Preparatory steps:
P0a	The DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0b	Network formation is triggered on DUT-ZC.
DUT successfully completes formation of a centralized network.
P1	THr1, THr2 and THe1 are factory new (For information: bdbNodeIsOnANetwork = FALSE) and powered off.

Test Procedure:
1a	Network steering is triggered on the DUT. 
1b	Power on THr1 and THr2. Network steering is triggered as far as possible simultaneously on THr1 and THr2.
2	Both THr1 and THr2 successfully complete the joining.
3a	Within bdbcTCLinkKeyExchangeTimeout THr1 and THr2 both request TC-LK update at the DUT.
3b	Conditional on 3a:
Within the PermitDuration, DUT sends APS Transport Key to THr1 and THr2, carrying the unique TC-LK.
3c	Conditional on 3b:
Within the PermitDuration, both THr1 and THr2 verify correctness of reception of the unique TC-LK.
3d	Conditional on 3c:
Within the PermitDuration, DUT responds to both THr1 and THr2 verifying correctness of the unique TC-LK.
3e	Note: the test passes if all the TC-LK update and verification steps (1-3d) for both joiners complete, even if some of the steps need to be retried by the joiners, (incl. association).
The test fails if at least one of the joiners does NOT complete all the steps within PermitDuration.
Verify correct APS security with two keys
Note: For steps 4a – 4d DUT has to still allow for joining. If the original PermitDuration of step 1a expires, re-trigger it at the DUT.
4a	If required, power off THr1. 
Power on THe1. 
Network steering is triggered on THe1.
4b	THr2 unicast to the DUT the protected APS Update-Device command for its THe1 child:
Since THr2 has a unique TCLK with the DUT, THr2 unicast to the DUT the APS Update-Device command for THe1 with the Status = 0x01 (Standard device unsecured join), correctly protected at the NWK level, and correctly protected at the APS level with the appropriate key type derived from the unique TC-LK of THr2; the APS security frame counter is incremented.
Since THr2 has a unique TCLK with the DUT, THr2 does NOT send the APS Update-Device command unprotected at the APS level.
4c	Trigger THe1 to perform network leave.
THe1 may send NWK Leave command with the sub-fields of the Options field set to: Request = 0b0 and Rejoin = 0b0.
THr2 may unicast to the DUT a correctly protected APS Update-Device command for its THe1 child, with the status Device left.
4d	If required, power off THr2. 
If required, power THr1 back on. THr1 silently resumes operation, using its previous settings.
If required, network steering is triggered on THe1.
4e	THr1 unicast to the DUT the protected APS Update-Device command for its THe1 child
Since THr1 has a unique TCLK with the DUT, THr1 unicast to the DUT the APS Update-Device command for THe1 with the Status = 0x01 (Standard device unsecured join), correctly protected at the NWK level, and correctly protected at the APS level with the appropriate key type derived from the default global TC-LK used by THr1; the APS security frame counter is incremented.
Since THr1 has a unique TCLK with the DUT, THr1 does NOT send the APS Update-Device command unprotected at the APS level.

Verification:
1a	DUT broadcasts Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime and TC_Significance = 0x01.
2	Both THr1 and THr2 successfully complete exchange of Beacon request, Beacon, Association Request, Association Response with the DUT.  
Within apsSecurityTimeOutPeriod DUT delivers the NWK key to the THr1 and THr2 in unicast APS Transport Key command, correctly protected with the default global Trust Center link key.  
3a	Within bdbcTCLinkKeyExchangeTimeout both THr1 and THr2 unicasts to the DUT an APS Request Key command, correctly protected at the NWK level with the NWK key and at the APS level with the default global Trust Center link key; with requested KeyType field set to 0x04 (Unique Trust Center Link Key) and Partner Address absent.
3b	Within PermitDuration, DUT unicasts to both THr1 and THr2 a correctly formatted APS Transport Key command, carrying a unique TC-LK (KeyType = 0x04) with a value other than that of the global default TC-LK; Destination address in the payload is set to the IEEE address of THr1 and THr2, respectively.
3c	Immediately upon reception of APS Transport Key with unique TC-LK, both THr1 and THr2 unicast to the DUT the APS Verify-Key command:
correctly protected at the NWK level with the NWK key;
NOT protected at the APS level, 
with the payload carrying:
StandardKeyType = 0x04;
Source address = IEEE address of THr1 or THr2, respectively;
Initiator Verify-Key Hash Value field of 16B present and calculated correctly.
3d	DUT unicasts to both THr1 and THr2 the  APS Confirm-Key command:
correctly protected at the NWK level with the NWK key; 
correctly protected at the APS level with the data key derived from the new unique TC-LK for THr1 or THr2, respectively;
Command payload with:
Status = SUCCESS;
StandardKeyType = 0x04;
Destination address set to IEEE address of the THr1.
4a	Make sure that THe1 successfully associates at THr2.
4b	DUT unicasts to the THr2 the APS Tunnel command carrying the APS Transport-Key command for THe1, correctly protected at the NWK level, and unprotected at the APS level.	
4d	Make sure that THe1 successfully associates at THr1.
4e	DUT unicasts to the THr1 the APS Tunnel command carrying the APS Transport-Key command for THe1, correctly protected at the NWK level, and unprotected at the APS level.



