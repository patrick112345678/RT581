6.1.8 1GTE#3: DR-TAR-TC-03E: Reset via NWK layer Leave command: additional tests; DUT: ZR
This test verifies the reset functionality of the NWK layer Leave command.
Specifically, behavior on reception of NWK Leave command from non-parent device and of NWK Leave command with Request sub-field of the Options field set to 0b0, is tested.


Required devices:
DUT - DUT capabilities according to its PICS: ZED or ZR, initiator or target
DUT supports reception of the NWK Leave command.
THr1 - TH ZR; complementing the application functionality of the DUT
THr2 - TH ZR


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THr1 is operational (bdbNodeIsOnANetwork = TRUE) on the distributed network A.
3 - DUT allows for reception of any NWK Leave request (nwkLeaveRequestAllowed = TRUE, 
nwkLeaveRequestWithoutRejoinAllowed = TRUE)	


Test preparation:
P1 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE), THr1 is its parent.
P2 - THr2 joins network A at the DUT.
To achieve this, THr1 can temporarily be switched off (network A is a distirbtued network).

