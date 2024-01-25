5.1.4 FB-PRE-TC-03A: Service discovery - client side tests
This test verifies the generation of the service discovery request commands and handling of the respective responses, if defined.


Required devices:
DUT - Device operational on a network: ZC, ZR, or ZED; 
supporting generation of service discovery request primitives

THr1 - TH ZR; operational on a network; 
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.

THe1 - TH Sleeping ZED,
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P0 - THr1, DUT and ZED are factory new and off.
P1 - THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE), formed by one of them. 
P2 - ZTe1 joins the network at THr1 (bdbNodeIsOnANetwork = TRUE).
P3 - Make sure the Binding table of the DUT is empty. 
If DUT uses groupcast binding, make sure the Groups table of THr1 and THe1 is empty. 


Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
