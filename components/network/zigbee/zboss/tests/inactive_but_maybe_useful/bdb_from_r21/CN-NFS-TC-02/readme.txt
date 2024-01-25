4.4.2 CN-NFS-TC-02: Network formation&steering triggered on non-factory-new ZC

This test verifies the network formation and steering procedure.
This test verifies the behavior of a non-factory-new ZC when network formation followed by network steering is triggered.  This test shall only be executed if the DUTТs PICS indicates support for such combination of commissioning procedures.
Successful completion of the applicable tests from sec.  and ќшибка: источник перЄкрестной ссылки не найден is a precondition for performing the current test.
The device under test takes the role of ZC; the other role can be performed by a test harness.


Required devices:
DUT - ZC of the centralized network
DUT has means to trigger network steering automatically following network formation


Additional devices
THr1 - TH ZR, capable of sending Beacon Request


Test preparation:
Preparatory step:
P1 - DUT is operational on a centralized network it formed (for information: bdbNodeIsOnANetwork = TRUE). The network parameters used by the DUT (PANId, EPID, radio channel) are registered.



Test Procedure:
 - In DUT-specific way network formation automatically followed by steering is triggered on the DUT.
 - Power on THr1.






