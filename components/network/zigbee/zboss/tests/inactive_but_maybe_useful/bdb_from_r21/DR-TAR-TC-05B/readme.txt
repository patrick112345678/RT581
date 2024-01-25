6.1.12 GTE#3: DR-TAR-TC-05B: Reset via Mgmt_Leave_req; DUT: ZED
This test verifies the reset functionality of the Mgmt_Leave_req command on a ZED.
Specifically, this test verifies that the DUT does retain NWK security frame counter, not leading to its re-use when rejoining the same network. 
Further, it tests that the DUT does NOT retain any other persistent data, at the network and application level, of the network it leaves, and that it restores the default values when joining a new network.


Required devices:
DUT - DUT capabilities according to its PICS: ZED, initiator or target
DUT supports reception of the NWK Leave command.
THc1 - TH ZC; Zigbee PAN Coordinator of network A
THr1c2 - TH ZCZR; Zigbee PAN Coordinator offormed distributed network B


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - The THc1 is operational (bdbNodeIsOnANetwork = TRUE) on the network A.
3 - The THc2r1 is operational (bdbNodeIsOnANetwork = TRUE) on the distributed network B.
4 - The DUT is operational on the network A (bdbNodeIsOnANetwork = TRUE), THc1 is its parent. 
5 - DUT allows for reception of Mgmt_Leave_req.
6 - There is no open network on any channel of the DUT’s bdbPrimaryChannelSet and bdbSecondaryChannelSet.


Test preparation:
P1 - The current value M of the NIB OutgoingFrameCounter parameter as used by the DUT is known.
It may be obtained from any properly secured Zigbee message the DUT sends over the Zigbee network. E.g., if the DUT is a ZC/ZR, it may be the Link Status message. If the DUT is a ZED, it may need to be triggered.

P2 - Conditional on DUT having at least one initiator endpoint X: 
at least one binding is created on the DUT with the initiator endpoint as source.
It may be configured via THc1, sending Bind_req or using another finding and binding procedure.

P3 - 1Conditional on DUT having at least one writable or reportable attribute on at least one endpoint X (as indicated in the DUT’s PICS) that is NOT reset via power cycle but IS reset via reset to factory defaults:
the current value Cxn and the factory default value Fxn of each PICS indicated attribute n of each endpoint X of the DUT is known, and Cxn != Fxn.
The current value Cxn can be changed by THc1 sending ZCL Write Attributes command.

P4 - Conditional on DUT having at least one reportable attribute on at least one endpoint Y (as indicated in the DUT’s PICS, it may be the same endpoint X in step P3)X, and it supports both reception of ZCL Read Reporting Configuration and the ZCL Configure Reporting command:
THc1 reads out the default reporting configuration of each PICS indicated reportable attribute i on endpoint Y, by sending ZCL Read Reporting Configuration; the DUT responds with ZCL Read Reporting Configuration Response command for the same reportable attribute i on endpoint Y, indicating Minimum reporting interval and Maximum reporting interval set to D1xyi and D2xyi, respectively.
The THc1 sends the ZCL Configure Reporting command each PICS indicated reportable attribute i on endpoint Y, with at least one Attribute reporting configuration record field, with the Direction field set to 0x00, and the Minimum reporting interval and Maximum reporting interval set to V1xyi and V2xyi different than D1yi and D2yi, respectively.
THc1 reads out the default reporting configuration of each PCIS indicated reportable attribute i on endpoint Y, by sending ZCL Read Reporting Configuration; the DUT responds with ZCL Read Reporting Configuration Response command indicating Minimum reporting interval and Maximum reporting interval set to V1xyi and V2xyi, respectively.
