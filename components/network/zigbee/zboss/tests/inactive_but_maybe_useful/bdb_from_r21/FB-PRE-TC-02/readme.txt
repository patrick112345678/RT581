5.1.3 B-PRE-TC-02: Device discovery – server side tests
This test verifies the reception of device discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the device discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the device discovery request primitives mandated by the BDB specification.
Additional optional devices

THe1 - TH Sleeping ZED
This device is only required if the DUT is a ZC/ZR, to test the child address caching
This role can be performed by a golden unit or a test harness


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P1 - DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE).
If DUT is a ZED, it shall join at the THr1, to prevent that discovery information caching by the parent is involved.
P2 - THe1 is factory new and off.


Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zr, zc or zed
 
