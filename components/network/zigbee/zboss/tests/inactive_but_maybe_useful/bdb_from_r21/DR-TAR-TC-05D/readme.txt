6.1.14 GTE#3: DR-TAR-TC-05D: Reset via Mgmt_Leave_req; additional tests
This test verifies the reset functionality of the Mgmt_Leave_req command.


Required devices:
DUT - DUT capabilities according to its PICS: ZED or ZR, initiator or target
DUT supports reception of the NWK Leave command.
THc1 - TH ZC; Zigbee PAN Coordinator of network A


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THc1 is operational (bdbNodeIsOnANetwork = TRUE) on the network A.
3 - DUT allows for reception of Mgmt_Leave_req.


Test preparation:
P1 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE).
P2 - THr1 is operational (bdbNodeIsOnANetwork = TRUE) on the network A.


Test procedure:
Before:
1a - Negative test: Mgmt_Leave_req without the last octet:
THc1 unicasts to the short address of the DUT a correctly protected Mgmt_Leave_req command, with Device Address set to the IEEE address of the DUT and the remaining payload fields (1octet) missing.
After
1a - DUT does NOT send NWK Leave command.
DUT continues operating on network A.

Before:
1c - Negative test: Mgmt_Leave_req command without the Device Address:
THc1 unicasts to the short address of the DUT a correctly protected Mgmt_Leave_req command, with Device Address field NOT present, with Rejoin = 0b0, Remove children = 0b0.

After:
ac - DUT does NOT send NWK Leave command.
DUT continues operating on network A.

Before:
1e - Negative test: broadcast Mgmt_Leave_req:
THc1 broadcasts a correctly protected Mgmt_Leave_req command, with Device Address field equal to the IEEE address of the DUT, with Rejoin = 0b0, Remove children = 0b0.

After:
1e - DUT does NOT send NWK Leave command.
DUT continues operating on network A.

Before:
2 - Positive test: ignoring relationship on reception of Mgmt_Leave_req: THr1 unicasts to the short address of the DUT a Mgmt_Leave_req command, with the Device Address set to IEEE address of the DUT: Rejoin = 0b0, Remove children = 0b0.
DUT does NOT remove its children (if any) from the network:

After:
2 - DUT does NOT send NWK Leave command with Request = 0b1 to THr1.
DUT processes Mgmt_Leave_Req:
DUT unicasts to THr1 a correctly protected Mgmt_Leave_rsp (ClusterID=0x8034), with Status=SUCCESS.
DUT broadcasts NWK Leave command, with MAC source address field set to the network address of the DUT, MAC destination address set to 0xffff, NWK header source IEEE address field present and carrying the IEEE address of the DUT,  NWK header destination address field set to 0xfffd, Radius = 0x01, correctly protected with the NWK key, with the Frame counter field of the AUX NWK header set to N >= M+1, with the sub-fields of the Options field set as follows: Rejoin = 0b0; Request = 0b0, Remove children = 0b0; Reserved = 0b0.
