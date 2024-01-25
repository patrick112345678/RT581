5.1.8 FB-PRE-TC-05: Groups cluster: F&B Initiator 
This is a test for a finding and binding initiator endpoint (bdbNodeIsOnANetwork = TRUE) implementing the BDB-mandated functionality of the Groups cluster. 
If a DUT has several initiator endpoints, the test should be repeated for each initiator endpoint.
This test verifies the operation of the Groups cluster client functionality, as mandated by the BDB; the other role should be performed by a test harness.


Required devices:
DUT - ZC/ZR/ZED, with at least one endpoint X being finding&binding initiator
Note: groupcast does not work for sleeping ZED.

THr1 - TH ZR with at least one endpoint Y being finding&binding target, implementing Groups cluster client behavior


Initial Conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - All devices are factory new and powered off until used.


Preparatory steps:
P1 - DUT and THr1 are operational on the same network, centralized (if ZC is involved) or distributed.



Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
