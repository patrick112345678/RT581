6.1.8 1GTE#3: DR-TAR-TC-03E: Reset via NWK layer Leave command: additional tests; DUT: ZR
This test verifies the reset functionality of the NWK layer Leave command.
Specifically, behavior on reception of NWK Leave command from non-parent device and of NWK Leave command with Request sub-field of the Options field set to 0b0, is tested.


Required devices:
DUT - ZR, initiator or target
DUT supports reception of the NWK Leave command.
THr1 - TH ZR; complementing the application functionality of the DUT
THr2 - TH ZR


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THr1 is operational (bdbNodeIsOnANetwork = TRUE) on the distributed network A.
3 - DUT allows for reception of any NWK Leave request (nwkLeaveRequestAllowed = TRUE, 
nwkLeaveRequestWithoutRejoinAllowed = TRUE)	


Test preparation:
P1	The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE), THr1 is its parent.
P2	Network steering is disabled (e.g. because it is explicitly closed by Mgmt_Permit_Joining_req with PermitDuration = 0x00).

Test procedure:
1a	Negative test: Rejoin = 0b0:
THr1 unicasts to the short address of the DUT a correctly protected NWK Leave command, with Rejoin = 0b0, Request = 0b0, Remove children = 0b0.

1b	NWK Leave command without the destination IEEE address:
THr1 unicasts to the short address of the DUT a correctly protected NWK Leave command, with the Destination IEEE Address sub-field of the NWK Frame Control field set to 0b0 and the destination IEEE address field NOT present, with Rejoin = 0b0, Request = 0b1, Remove children = 0b0.
 
2a	Repeat preparatory step P1.

2b	During the PermitDuration extended in P1 by the joining DUT:
Switch off THr1.
Power on THr2. 
Trigger network steering on THr2.

2c	NWK Leave request sent by a child: 
THr2 unicasts to the short address of the DUT a correctly protected NWK Leave command, with Rejoin = 0b0, Request = 0b1, Remove children = 0b0.

Verification:
1a	DUT does NOT send NWK Leave command.
DUT remains operational on the network A.

1b	DUT broadcasts NWK Leave command, with the sub-fields of the Options field set as follows: Rejoin = 0b0; Request = 0b0, Remove children = 0b0; Reserved = 0b0.
DUT does NOT send NWK Rejoin request for network A.

2b	THr2 joins network A at the DUT.

2c	DUT broadcasts NWK Leave command, with the sub-fields of the Options field set as follows: Rejoin = 0b0; Request = 0b0, Remove children = 0b0; Reserved = 0b0.
DUT does NOT send NWK Rejoin request for network A.

