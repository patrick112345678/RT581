5.3.8 B-INI-TC-07A: Finding and binding follows automatically on network steering: FN device
This test verifies the behavior of a factory-new device, on which a sequence of network steering automatically followed by finding and binding is triggered. 
This test shall only be performed if such a combination of commissioning procedures is supported.


Required devices:
DUT - ZR or ZED, capable of joining a network 

Additional devices:
THr1 - TH ZR, capable of forming a distributed network


Test preparation:
P1 - THr1 is powered on and triggered to form a network on one of the primary channels supported by the DUT. 
P2 - DUT is placed at 1m distance from the THr1 and powered on.
P3 -The endpoints supported by the DUT are known, incl. their F&B initiator or target role.


Test Procedure:
 - Network steering is triggered on THr1.
In DUT-specific way network steering automatically (i.e. without user interaction) followed by finding&binding is triggered on the DUT.
 - DUT and THr1 successfully complete network steering procedure, incl. delivery on NWK key.



Additional info:
to run test type - runng <dut_role> <fb>,
where dut_role can be zr or zed
and fb can be initiator or target
e.g: runng zr target or runng zed initiator
