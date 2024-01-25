5.3.7 FB-INI-TC-06B: Finding and binding follows automatically on network formation&steering: NFN device
This test verifies the behavior of a non-factory-new device, on which a sequence of network formation, automatically followed by network steering automatically followed by finding and binding is triggered. 
This test shall only be performed if such a combination of commissioning procedures is supported.


Required devices:
DUT - ZC or ZR, capable of network formation. 

Additional devices:
THr1 - TH ZR, capable of joining a centralized or a distributed network.



Test preparation:
P1 - DUT is powered on. DUT is triggered to form a network.
P2 - THr1 is placed at 1m distance from the DUT.
P3 - The endpoints supported by the DUT are known, incl. their F&B initiator or target role.



Test Procedure:
 - In DUT-specific way network formation automatically followed (i.e. without user interaction) by steering automatically followed by finding&binding is triggered on the DUT.
 - Within AssociationPermit time window, power on THr1 and trigger network steering. THr1 begins network steering procedure for a node not on a network, sending Beacon Request frames.


Additional info:
to run test type - runng <dut_role> <fb_role>,
where dut_role can be zc or zr
and fb_role can be initiator or target
i.e: runng zc target
