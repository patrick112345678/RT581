5.1.11 FB-PRE-TC-08: Attribute Reporting
This test verifies the attribute reporting behavior of an initiator device.  
The DUT is the initiator device; the other role has to be performed by a TH.


Required devices:
DUT - Finding & binding initiator
ZC, ZR or ZED, according to device logical type

THr1 - TH ZR, Finding & binding target

Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - THr1 and DUT are factory new and off.


Preparatory steps:
P1 - THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE).
P2 - THr1 obtains a list of active endpoints of the DUT, and for each of the active endpoints, a Simple Descriptor. The THr1 determines a list of operational initiator clusters with mandatory reportable attribute.



Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
