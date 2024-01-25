CN-NST-TC-09: joining a legacy (r20 or older) ZC, DUT: ZR or ZED
This test verifies the network steering behavior of a ZR/ZED joining a legacy TC.
The device takes the role described in its PICS; the other role can be performed by a test harness or a golden unit.
Successful completion of tests 99GTE#3: CN-NST-TC-01A: joining at ZC; DUT: joiner ZR or ZEDand 103GTE#3: CN-NST-TC-01B: ZR joining at ZC; DUT: ZC is pre-requisite for performing the current test.


Required devices:
DUT - ZR or ZED 
THc1 - Legacy (r20 or older) ZC of a network; 
ZC uses default global TC-LK for NWK key delivery.


Preparatory step:
P1 - Turn on THc1. Network formation is triggered on THc1. THc1 successfully forms a centralized network.
P2 - Network steering is triggered on the THc1.


Test Procedure:
 - DUT is placed in range of THc1 and powered on. Network steering is triggered on the DUT. 
