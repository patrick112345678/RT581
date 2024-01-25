7.2.6 CS-NFS-TC-05B: ZC behavior when a known node joins, DUT: ZC with bdbJoinUsesInstallCodeKey = FALSE
This test verifies the following behavior: If a node that has already exchanged its Trust Center link key attempts to join the Trust Center with bdbJoinUsesInstallCodeKey =FALSE for a second time, the Trust Center SHALL allow the node to join but in a fresh state and use the dTCLK.


Required devices:
DUT - ZC 
THr1 - TH ZR in the role of joiner


Initial conditions:
1) A packet sniffer shall be observing the communication over the air interface.
2) For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
The value of the DUT’s bdbTCLinkKeyExchangeAttemptsMax attribute is known (default = 0x3).


Test preparation:
P0a - DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0b - Network formation is triggered on DUT.
DUT successfully completes formation of a centralized network.
P1 - The THr1 is factory new (For information: bdbNodeIsOnANetwork = FALSE). 
P2 - Network steering is triggered on the DUT. Network steering is triggered on the THr1. DUT and THr1 successfully complete network steering procedure, including: NWK delivery, protected with the dTCLK;
TCLK update;
P3 - Reset THr1.
P4 - DUT’s bdbJoinUsesInstallCodeKey =FALSE. 


Test Procedure:
 - Network steering is triggered on the DUT.
 - Network steering is triggered on the THr1. THr1 sends Beacon Request at least on the operational channel of the DUT.
 
