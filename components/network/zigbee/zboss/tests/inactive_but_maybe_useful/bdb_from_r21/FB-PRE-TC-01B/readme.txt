5.1.2 FB-PRE-TC-01B: Device discovery – client side additional tests
This test contains negative tests for the handling of the device discovery responses, if defined.


Required devices:
DUT - Device operational on a network, ZC, ZR, or ZED; 
supporting generation of device discovery request primitives as mandated by the BDB specification

THr1 - TH ZR operational on a network; 
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster.

THe1 - TH Sleeping ZED,
supports reception of all the device and service discovery request primitives supported by the DUT, and having at least one matching cluster. This role can be performed by a golden unit or a test harness

Additional devices:
THc1 - TH ZC, capable forming a centralized network,  
Presence of a separate THc1 node required, to prevent DUT adding THr1 to its neighbor table (as a parent) 
This role can be performed by a golden unit or a test harness


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.


Test preparation:
P0 - THc1, THr1, DUT and THe1 are factory new and off.
P1 - THr1 joins the network at THc1 (bdbNodeIsOnANetwork = TRUE). 
Switch THr1 off (to avoid that it is added to the neighbor table of the DUT).
P2 - DUT joins the network at THc1 (bdbNodeIsOnANetwork = TRUE).
P3 - Switch DUT off. Switch THr1 on. THe1 joins at the THr1. Switch DUT on.


Test Procedure:
3a) IEEE_addr_req command for a THe1:
In an application-specific manner, DUT is triggered to send IEEE_addr_req command for the THe1.
3b) Negative test: wrong NWK address in IEEE_addr_rsp:
    THr1 unicasts to the DUT a correctly formatted IEEE_addr_rsp, with the command payload fields: 
    Status = SUCCESS;
     IEEEAddrRemoteDev = IEEE address of the THe1;
     NWKAddrRemoteDev = network address of the THr1; 
     the fields NumAssocDev = 0x00,  StartIndex = 0x00, NWKAddrAssocDevList absent.
4a) IEEE_addr_req command for a THe1:
In an application-specific manner, DUT is triggered to send IEEE_addr_req command for the THe1.
4b) Negative test: wrong status in IEEE_addr_rsp:
THr1 unicasts to the DUT a correctly formatted IEEE_addr_rsp, with the command payload fields: 
Status = DEVICE_NOT_FOUND;
 IEEEAddrRemoteDev = IEEE address of the THe1;
 NWKAddrRemoteDev = network address of the THe1; 
 the fields NumAssocDev,  StartIndex, and NWKAddrAssocDevList absent.



Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zr or zed

In current implementation THr1 forwards IEEE address request to THe1, but does not forward response to DUT;
DUT and THe1 starts finding and binding

This test is hard to run on nsng.
