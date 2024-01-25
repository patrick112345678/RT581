3.6.3 Negative test: DN-KTU-TC-03: NWK key for distributed security network protected with IC
This negative test covers rejection of network key for a distributed network, when protected with incorrect link key. 
This test verifies the operation of the device attempting to join a distributed network; the other role has to be performed by a test harness.


3.6.3.1 Required devices
DUT - ZR/ZED, capable of joining a distributed network
THr1 - TH ZR, capable of distributed network formation
TH is modified to send the Transport Key command with the NWK key protected with the IC-derived link key 
