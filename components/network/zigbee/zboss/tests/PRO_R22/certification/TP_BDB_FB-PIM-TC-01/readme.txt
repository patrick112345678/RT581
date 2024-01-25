5.5.1 FB-PIM-TC-01: Operation with ProfileID matching rules
This test verifies the endpoint profile matching rules on reception of operational (ZCL) commands. Specifically, it verifies the DUT’s capability of receiving messages addressed to the legacy profiles

Required devices:
DUT - Finding & binding target; ZC, ZR or ZED, according to device role
THr1 - TH ZR; Finding & binding target

Test preparation:
P1 - THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE).
P2 - THr1 gets a list of active endpoints of the DUT, and for each of the active endpoints, a Simple Descriptor. The THr1 determines the total number N and a list of clusters that are initiators of operational transactions.

Test Procedure:
1	THr1 sends to a target endpoint of the DUT a command for a target cluster, correctly formatted and protected, using the APS ProfileID = 0x0104;
Note: preferably, the command is selected such that its execution can be verified, either by over the air behavior (e.g. response, default response or changed attribute value), or by a user observing the DUT. Preferably, the command requires a response.
2	Wildcard profile:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using wildcard ProfileID 0xffff.
3	Negative test: ZSE profile:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using ZSE ProfileID 0x0109.
4	Negative test: GW profile:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using GW ProfileID 0x7f02.
5	Negative test: MSP profile:
THr1 sends to a target endpoint of the DUT a message for a target cluster, using any MSP ProfileID NOT support by the DUT.

Verification:
1	DUT executes the command.
Conditional on the triggering message requiring a response:
DUT generates a response-style command, DUT using the same ProfileID as in the triggering message.
2	DUT executes the command.
Conditional on the triggering message requiring a response:
DUT generates a response-style command, DUT using the ProfileID as in the Simple Descriptor of DUT’s target endpoint (as in P2).
3	DUT does not execute the command.
4	DUT does not execute the command.
5	DUT does not execute the command.

Additional info:
to run test type - runng <dut_role>
where dut_role can be zc, zed or zr
e.g: runng zc
