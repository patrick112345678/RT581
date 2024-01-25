5.1.9 FB-PRE-TC-06: Groups cluster: TARGET
This is a test to check that a finding and binding target (bdbNodeIsOnANetwork = TRUE) implements the BDB-mandated functionality of the Groups cluster. 
If a DUT has several target endpoints, the test shall be repeated for each target endpoint.
This test verifies the operation of the Groups cluster server functionality, as mandated by the BDB; the other role shall be performed by a test harness, because of the inclusion of the negative tests.


Required devices:
DUT - ZC/ZR/non-sleeping ZED, with at least one endpoint X being finding&binding target

THr1 - TH ZR, with at least one endpoint Y being finding&binding initiator, implementing Groups cluster client behavior 

Initial Conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - All devices are factory new and powered off until used.


Test preparation:
P1 - DUT and THr1 are operational on the same network, centralized (if ZC is involved) or distributed.


Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
