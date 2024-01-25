6.1.4 DR-TAR-TC-03A: Reset via NWK layer Leave command: DUT: ZR
This test verifies the reset functionality of the NWK layer Leave command on a Zigbee Router.
Specifically, this test verifies that the DUT does retain NWK security frame counter, not leading to its re-use when rejoining the same network. 
Further, it tests that the DUT does NOT retain any other persistent data, at the network and application level, of the network it leaves, and that it restores the default values when joining a new network.
It tests NWK Leave execution on centralized and distributed network.


Required devices:
DUT - DUT capabilities according to its PICS: ZR, initiator or target
DUT supports reception of the NWK Leave command.

THc1 - TH ZC; Zigbee PAN Coordinator of network A, complementing the application functionality of the DUT
This role has to be performed by a TH
THe1 - TH ZED; complementing the application functionality of the DUT
THr1 - TH ZR; ZR that created the distributed network B, complementing the application functionality of the DUT
This role has to be performed by a TH
THr2 - TH ZR: complementing the application functionality of the DUT


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THc1 is operational (bdbNodeIsOnANetwork = TRUE) on the network A.
3 - The THr1 and THr2 areis operational (bdbNodeIsOnANetwork = TRUE) on the distributed network B. 
4 - There is no open network on any channel of the DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet.
5 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE). 
6 - THe1 joins network A at the DUT.
7 - DUT allows for reception of any NWK Leave request (nwkLeaveRequestAllowed = TRUE, 
nwkLeaveRequestWithoutRejoinAllowed = TRUE)	

