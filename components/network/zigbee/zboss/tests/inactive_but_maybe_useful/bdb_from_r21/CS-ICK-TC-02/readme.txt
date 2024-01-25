7.1.3 CS-ICK-TC-02: Additional tests for IC-based LK, DUT: ZC
This test covers negative tests for joining centralized network with the IC-based link key; incl. network key delivery and TC-LK update. 
This test verifies the behavior of the ZC accepting the joiner with IC-based link key; the other role has to be performed by a test harness.
Successful completion of test CS-ICK-TC-01 is precondition for performing the current test.


Required devices:
DUT - ZC, capable of centralized network formation
ZC is capable of accepting ICs 
THr1 - TH ZR, capable of joining a centralized network and having IC
THr2 - TH ZR, capable of joining a centralized network


Preparatory steps:
P1 - DUT is powered on and triggered to form a centralized network. DUT successfully forms a centralized network. 


Test Procedure:
 - Joiner’s IC not entered at the ZC (bdbJoinUsesInstallCodeKey test):
   - Network steering is triggered on the DUT.
Conditional on availability of a tool/app to enter the IC at the ZC: 
Attempt entering incorrect IC into ZC:
 - In a DUT-specific way enter incorrect IC code for THr1 (one with non-matching CRC).
 
If THr1 successfully joined at step 1: reset DUT, repeat preparatory step P1.
 - Negative tests for correct IC1 (of THr1).
   - In a DUT-specific way enter correct IC-based unique TCLK for THr1 and THr1’s IEEE address into the DUT.
   - Network steering is triggered on the DUT. DUT and THr1 successfully complete MAC association.
   - Negative test: Within bdbTrustCenterNodeJoinTimeout THr1 sends APS Request Key command, correctly formatted, but protected at the APS level with a key derived from dTCLK.
   - Negative test: Within bdbTrustCenterNodeJoinTimeout:
Enter IC2-based unique TCLK2, together with IEEE address NOT corresponding to THr1 at the DUT. 
THr1 sends APS Request Key command, correctly formatted but protected at the APS level with a unique TCLK2.
   - Positive test: 
Still within bdbTrustCenterNodeJoinTimeout, THr1 sends APS Request Key correctly protected with a key derived from THr1’s unique TCLK.

Conditional on DUT’s bdbJoinUsesInstallCodeKey = FALSE:
check that the DUT-ZC can support joining with IC-based TCLK and dTCLK.
 - Positive test: THr2 joins using dTCLK:
Turn THr2 on. Network steering is triggered on the THr2. DUT and THr2 successfully complete MAC association.
 - THr2 sends APS Request Key command, correctly formatted and protected at the APS level with the dTCLK.



Additional information:
Script:
 - in first step script (runng.sh) runs dut fow which bdbJoinUsesInstallCodeKey = FALSE
 - then run thr1
 - after some delay run thr2 and waits
 - at second step script run dut for which bdbJoinUsesInstallCodeKey = TRUE
 - then run THr1 and waits for join timeout
 - then waits some delay and checks that invalid install code does not added to ic-table on the dut
 - after this step script run thr1 again and wait for some delay

DUT2 (dut_using_ic):
 - after first network steering dut does not enter ic-codes
 - wait for DELAY1 and adds invalid ic-code
 - wait fot DELAY2 and enters valid ic-code for thr1
 - see test_common.h for exact values of these delays


THR1:
 - grep thr1 logs by "KEY:" to see tc link keys derived from install codes.


