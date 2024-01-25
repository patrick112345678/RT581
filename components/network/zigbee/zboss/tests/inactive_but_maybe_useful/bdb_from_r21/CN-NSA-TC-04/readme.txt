4.5.6 N-NSA-TC-04: Joining both network types on the same trigger; DUT: joiner (ZR/ZED)
This test checks fi the DUT can join both centralized and distributed security network using exactly the same trigger.


Required devices:
DUT - ZR/ZED, attempting to join a network
THc1 - TH ZC 
THr1 - TH ZR



Preparatory steps:
P1 - The value of DUT’s bdbPrimaryChannelSet is known. It is not 0x00000000.
P2 - TH is powered on and triggered to form a network; the network type (distributed/centralized) is randomly chosen without informing the DUT vendor. 
     TH successfully forms a network on a primary channel supported by the DUT. 
P3 - Network steering is triggered on the TH.
Note: the test steps have to be performed while the network of TH is open for joining. 
P4 - DUT is powered on.


Test Procedure:
 - Power on the THr1.
Network formation is triggered on the THr1. 
THr1 successfully forms a distributed network on a primary channel supported by the DUT.
Network steering is triggered on the THr1. 
 - Via a user or application trigger, network steering is triggered on the DUT.
 - Power off THr1.
   Reset the DUT.
 - Power on the THc1.
Network formation is triggered on the THc1. 
THc1 successfully forms a centralized network on a primary channel supported by the DUT.
Network steering is triggered on the THc1.
 - Via the same user or application trigger as used in step 2a above, network steering is triggered on the DUT.




Additional info:
to run test type - runng <dut_role>
where dut_role can be zed or zr
e.g: runng zc
