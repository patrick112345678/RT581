6.1.6 DR-TAR-TC-03C: Reset via NWK layer Leave command: DUT: ZC
This test verifies that the Zigbee PAN Coordinator does NOT accept NWK layer Leave command.


Required devices:
DUT - DUT capabilities according to its PICS: ZC, 
THr1 - TH ZR


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation
P1 - The DUT and THr1 are operational (bdbNodeIsOnANetwork = TRUE) on the network A.

Test procedure:
1	TH1 is triggered to send to the DUT the NWK Leave command. 
THr1 unicasts to the short address of the DUT a NWK Leave command, with the fields of the Command options field set to: Rejoin = 0b0, Request = 0b1, Remove children = 0b0.

Verification:
1. DUT does NOT send NWK Leave command.
DUT remains operational on the network A.

