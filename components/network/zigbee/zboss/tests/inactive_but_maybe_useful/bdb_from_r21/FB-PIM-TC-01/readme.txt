5.5.1 FB-PIM-TC-01: Operation with ProfileID matching rules
This test verifies the endpoint profile matching rules on reception of operational (ZCL) commands. Specifically, it verifies the DUT’s capability of receiving messages addressed to the legacy profiles


Required devices:
DUT - Finding & binding target; ZC, ZR or ZED, according to device role
THr1 - TH ZR; Finding & binding target


Test preparation:
P1 - THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE).
P2 - THr1 gets a list of active endpoints of the DUT, and for each of the active endpoints, a Simple Descriptor. The THr1 determines the total number N and a list of clusters that are initiators of operational transactions.


Test Procedure:
 - THr1 sends to a target endpoint of the DUT a command for a target cluster, correctly formatted and protected, using the APS ProfileID = 0x0104;
Note: preferably, the command is selected such that its execution can be verified, either by over the air behavior (e.g. response, default response or changed attribute value), or by a user observing the DUT. Preferably, the command requires a response.
 - Other legacy profiles:
Repeat the test for each of the legacy ProfileIDs {0x0101, 0x0102, 0x0103, 0x0105, 0x0106, 0x0107, 0x0108}:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using one of the legacy ProfileIDs (0x0101 – 0x0103, 0x0105 - 0x0108).
 - Wildcard profile: THr1 sends to a target endpoint of the DUT a message for a target cluster, using wildcard ProfileID 0xffff.
 - Negative test: ZSE profile: THr1 sends to a target endpoint of the DUT a message for a target cluster, using ZSE ProfileID 0x0109.
 - Negative test: GW profile: THr1 sends to a target endpoint of the DUT a message for a target cluster, using GW ProfileID 0x7f02.
 - Negative test: MSP profile:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using any MSP ProfileID NOT support by the DUT.
 

Additional info:
to run test type - runng <dut_role>
where dut_role can be zc, zed or zr
e.g: runng zc
