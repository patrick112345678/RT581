6.1.6 DR-TAR-TC-03C: Reset via NWK layer Leave command: DUT: ZC
This test verifies that the Zigbee PAN Coordinator does NOT accept NWK layer Leave command.


Required devices:
DUT - DUT capabilities according to its PICS: ZC, 
THr1 - TH ZR


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation
P1 - The DUT and THr1 are operational (bdbNodeIsOnANetwork = TRUE) on the network A.
