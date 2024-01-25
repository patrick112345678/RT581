DR-TAR-TC-05C: Reset via Mgmt_Leave_req command: DUT: ZC
This test verifies that the Zigbee PAN Coordinator does NOT accept Mgmt_Leave_req command.


Required devices:
DUT - DUT capabilities according to its PICS: ZC 
THr1 - TH ZR


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P1 - The DUT and THr1 are operational (bdbNodeIsOnANetwork = TRUE) on the network A.


Test procedure:
Before:
1 - THr1 is triggered to send to the DUT the Mgmt_Leave_req command. 
THr1 unicasts to the short address of the DUT a Mgmt_Leave_req command, with Device Address set to the IEEE address of the DUT and: Rejoin = 0b0, Remove children = 0b0.

After:
DUT does NOT send NWK Leave command.
DUT does NOT send Mgmt_Leave_rsp or DUT sends Mgmt_Leave_rsp with Statis other than SUCCESS.
DUT remains operational on the network A.

