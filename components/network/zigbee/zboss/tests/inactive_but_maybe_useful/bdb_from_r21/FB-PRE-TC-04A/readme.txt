5.1.6 FB-PRE-TC-04A: Service discovery - server side tests 
This test verifies the reception of discovery request commands mandated by the BDB specification and generation of respective response.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; supporting reception of all the service discovery request primitives mandated by the BDB specification.

THr1 - TH ZR operational on a network; supporting transmission of all the service discovery request primitives mandated by the BDB specification.


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.

Test preparation:
P1 - DUT and THr1 are operational on a network (bdbNodeIsOnANetwork = TRUE).
