4.3.9 CN-NST-TC-08: Network steering on a factory-new ZC; DUT: ZC

This is a test to validate that network steering is not possible on a ZC which is not operational on a network. 
1This test shall only be performed if the DUT ZC offers an option (application or user trigger) to perform network steering only, independent of the current device state (not-/factory-new).
This test cannot be performed, if the DUT does not offer the option to perform network steering only, i.e. if network steering, if unsuccessful, will be followed by an attempt to form a network.


Required devices:
DUT - ZC, capable of centralized network formation


Test Procedure:
 - Power on DUT
 - Trigger network steering on the DUT

