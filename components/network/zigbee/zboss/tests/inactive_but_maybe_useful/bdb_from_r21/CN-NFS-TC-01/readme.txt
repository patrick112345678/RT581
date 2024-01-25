4.4.1 CN-NFS-TC-01: Network formation followed by steering on factory-new ZC; DUT: ZC

This test verifies the network formation automatically followed by a steering procedure.
It’s an optional test, only to be executed if the DUT’s PICS indicates support for such combination of commissioning procedures.

This test verifies the behavior of the factory new ZC when network formation is automatically followed by network steering.  
Successful completion of the applicable tests from sec. Centralized network formation; DUT: ZC and Centralized network: network steering is a condition for performing the current test.
The device takes the role described in its PICS; the other role can be performed by a test harness.


Required devices
DUT - ZC of the centralized network
DUT has means to trigger network formation automatically followed by network steering

Additional devices:
THr1 - TH ZR, capable of sending Beacon Request

Test Procedure:
 - Power on DUT
 - In DUT-specific way network formation automatically followed by steering is triggered on the DUT
 - DUT forms a network
 - DUT automatically (i.e. without user interaction) enables network steering.
 - Power on THr1
