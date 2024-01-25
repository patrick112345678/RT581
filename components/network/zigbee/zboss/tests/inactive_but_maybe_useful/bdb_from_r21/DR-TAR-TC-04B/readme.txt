6.1.10 DR-TAR-TC-04B: Reset via local interaction (if supported); DUT: ZED
This test verifies the reset functionality of the NWK layer Leave command.


Required devices:
DUT - DUT capabilities according to its PICS: ZED initiator or target
DUT supports reception of the NWK Leave command.

THc1 - TH ZC; Zigbee PAN Coordinator of network A

THr1 - TH ZR; ZR forming a distributed network; complementing the application functionality of the DUT


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THc1 is operational (bdbNodeIsOnANetwork = TRUE) on the network A.
3 - The THr1 is operational (bdbNodeIsOnANetwork = TRUE) on the distributed network B.
4 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE). 
5 - There is no other open network on any of the DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet.
