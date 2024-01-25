CS-NFS-TC-05A: ZC behavior when a known node joins, DUT: ZC with bdbJoinUsesInstallCodeKey = TRUE
This test verifies the following behavior: If a node that has already exchanged its Trust Center link key attempts to join the Trust Center with bdbJoinUsesInstallCodeKey =TRUE for a second time, the Trust Center SHALL allow the node to join but in a fresh state and use the initial link key appropriate for the node when transferring the network key, i.e. the IC-derived TCLK, if stored.

Required devices
DUT - ZC

THr1 - TH ZR in the role of joiner
THr1 has an IC

Initial conditions:
1	A packet sniffer shall be observing the communication over the air interface.
2	For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
The value of the DUT’s bdbTCLinkKeyExchangeAttemptsMax attribute is known (default = 0x3).

Preparatory steps:
P0a	DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0b	Network formation is triggered on DUT.
DUT successfully completes formation of a centralized network.
P1	The THr1 is factory new (For information: bdbNodeIsOnANetwork = FALSE). 
P2a	IC of the THr1 is entered into the DUT.
P2b	Network steering is triggered on the DUT. Network steering is triggered on the THr1. DUT and THr1 successfully complete network steering procedure, including: 
NWK delivery, protected with the THr1’s IC-derived preconfigured LK;
TCLK update.   
P3	Reset THr1 to factory new. If THr1 sends a NWK Leave command, it shall not be received by the DUT.
P4	DUT’s bdbJoinUsesInstallCodeKey =TRUE. 
It is known if the DUT does store the original IC.

Test Procedure:
1	Network steering is triggered on the DUT.
2	Network steering is triggered on the THr1.
THr1 sends Beacon Request at least on the operational channel of the DUT.

Verification:
1	DUT enables AssociationPermit.
DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
2	DUT and THr1 successfully complete beacon and association.
For information: DUT resets the TCLK of THr1.
DUT does NOT send the NWK key to the THr1 in unicast APS Transport Key command, protected with the unique Trust Center link key of THr1, as established in P2b.
Conditional on DUT storing the original IC of THr1 (BDB PICS item BKN2 = TRUE):
DUT sends the NWK key to the DUT in unicast APS Transport Key command, protected with the unique IC of THr1, as entered in P2a.
Conditional on DUT NOT storing the original IC of THr1 (BDB PICS item BKN2 = FALSE):
DUT does NOT send APS Transport Key command to THr1.

