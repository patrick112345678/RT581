7.1.4 CS-ICK-TC-03: Additional tests for IC-based LK, DUT: joiner (ZR/ZED)
This test covers negative tests for joining centralized network with the IC-based link key; incl. network key delivery and TC-LK update. 
This test verifies the behavior of the joiner with IC-based link key; the other role has to be performed by a test harness.
Successful completion of test CS-ICK-TC-01 is precondition for performing the current test.


Required devices:
DUT - ZR/ZED, capable of joining a centralized network and having IC 
THc1 - TH ZC, capable of centralized network formation
ZC is capable of accepting ICs


Preparatory steps:
P1 - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network. 
P2 - IC-based unique TCLK and IEEE address of the DUT is entered into THc1.


Test Procedure:
 - THc1 sends NWK key protected with TCLK derived from another IC: Network steering is triggered on the DUT, and, if required, the THc1.
THc1 and DUT successfully complete exchange of Beacon Request, Beacon, MAC Association Request/Response.
Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THc1 unicasts to the DUT an APS Transport Key command, protected at the APS level with a TC-LK derived from another IC.
 - Conditional on DUT attempting to rejoin the network of the THc1: Positive test: THc1 sends NWK key  protected with correct IC:
Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THc1 unicasts to the DUT an APS Transport Key command, correctly protected at the APS level with a TC-LK derived from DUT’s IC.
 - Reset DUT to factory-new state.
 - Negative test: THc1 sends unique TC-LK, protected with dTCLK:
Network steering is triggered on the DUT, and, if required, the THc1.
THc1 and DUT successfully complete association, and NWK key delivery, protected with the TC-LK derived from DUT’s IC.
Within bdbcTCLinkKeyExchangeTimeout after reception of APS Request Key command with KeyType field set to 0x04 (Unique Trust Center Link Key), THc1 unicasts to the DUT an APS Transport Key message, correctly formatted, but protected at the APS level with a key derived from default global TC-LK.
 - Negative test: THc1 sends unique TC-LK, protected with TCLK derived from another IC:
Within bdbcTCLinkKeyExchangeTimeout after reception of the second APS Request Key command with KeyType field set to 0x04 (Unique Trust Center Link Key), THc1 unicasts to the DUT an APS Transport Key message, correctly formatted, but protected at the APS level with a key derived from another IC.
 - Positive test:
Within bdbcTCLinkKeyExchangeTimeout after reception of the third APS Request Key command with KeyType field set to 0x04 (Unique Trust Center Link Key), THc1 unicasts to the DUT an APS Transport Key message, correctly formatted  and protected at the APS level with a key derived from DUT’s IC.


Additional information:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zed
