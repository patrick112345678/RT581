6.1.9 GTE#3: DR-TAR-TC-04A: Reset via local interaction (if supported); DUT: ZR
This test verifies the reset functionality of a local reset functionality on the DUT, if supported by the DUT.
Specifically, this test verifies that the DUT does retain NWK security frame counter, not leading to its re-use when rejoining the same network. 
Further, it tests that the DUT does NOT retain any other persistent data, at the network and application level, of the network it leaves, and that it restores the default values when joining a new network.


Required devices:
DUT - DUT capabilities according to its PICS: ZR, initiator or target
DUT supports reception of the NWK Leave command.

THc1 - TH ZC; Zigbee PAN Coordinator of network A complementing the application functionality of the DUT

THr1 - TH ZR: complementing the application functionality of the DUT

THr2 - TH ZR; ZR forming a distributed network; complementing the application functionality of the DUT


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THc1 and THr1 areis operational (bdbNodeIsOnANetwork = TRUE) on the network A.
3 - The THr2 is operational (bdbNodeIsOnANetwork = TRUE) on the distributed network B.
4 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE). 
5 - There is no other open network on any of the DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet.

