4.4.3 CN-NFS-TC-03: Network formation&steering triggered on non-factory-new ZR

This test verifies the network formation and steering procedure.
This test verifies the behavior of a non-factory-new ZR when network formation is automatically followed by network steering.  This test shall only be executed if the DUTТs PICS indicates support for such combination of commissioning procedures.
Successful completion of the applicable tests from sec.  and ќшибка: источник перЄкрестной ссылки не найден is a precondition for performing the current test.
The device under test takes the role of ZC; the other role can be performed by a test harness.


Required devices:
DUT - ZR in a centralized network.
DUT has means to trigger network steering automatically following network formation

Additional devices:
THr1 - TH ZR, capable of sending Beacon Request
THc1 - TH ZC of the centralized network.


Preparatory step:
P1 - THc1 is operational on a network it formed (for information: bdbNodeIsOnANetwork = TRUE). The network parameters used by the DUT (PANId, EPID, radio channel) are registered.
P2 - DUT joins and is operational on the network of THc1 (for information: bdbNodeIsOnANetwork = TRUE).


Test procedure:
 - In DUT-specific way network formation automatically followed by steering is triggered on the DUT.
 - Power on THr1.


