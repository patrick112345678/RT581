TP-CST-TC-06: Basic cluster with server as DUT

Test basic cluster and read/write attributes (server side). This cluster is typically used by a
configuration device to retrieve general information about another device within a lighting network.


ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).

Prepare steps
1.  Start ZR and ZED both as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR responds with ZLL.Commissioning.Scan response.
5.  ZED starts Start New Network procedure with all needs parameters
6.  ZR start new network and response ZED.
7.  ZED rejoin this network.

Test steps
1.  ZED unicasts a ZCL read attributes command frame for the ZCLVersion attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. ZCLVersion attribute has the value 0x01.
2.  ZED unicasts a ZCL read attributes command frame for the ApplicationVersion attribute to ZR. 
    ZR unicasts a ZCL read attributes response command frame to ZED. ApplicationVersion attribute has a 
    manufacturer specific value.
3.  ZED unicasts a ZCL read attributes command frame for the StackVersion attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. StackVersion attribute has a
    manufacturer specific value.
4.  ZED unicasts a ZCL read attributes command frame for the HWVersion attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. HWVersion attribute has a
    manufacturer specific value.
5.  ZED unicasts a ZCL read attributes command frame for the ManufacturerName attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. ManufacturerName attribute contains the manufacturer name string.
6.  ZED unicasts a ZCL read attributes command frame for the ModelIdentifier attribute to ZR. ZR unicasts 
    a ZCL read attributes response command frame to ZED. ModelIdentifier attribute contains the model identifier string.
7.  ZED unicasts a ZCL read attributes command frame for the DateCode attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. DateCode attribute has a manufacturer specific value.
8.  ZED unicasts a ZCL read attributes command frame for the PowerSource attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. PowerSource attribute has a value specified in the ZCL.
9.  ZED unicasts a ZCL read attributes command frame for the SWBuildID attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. SWBuildID attribute contains a manufacturer specific string.
10. ZED unicasts a ZCL read attributes command frame for the ZCLVersion and PowerSource attribute to ZR. ZR unicasts a 
    ZCL read attributes response command frame to ZED. ZCLVersion attribute has the value 0x01. PowerSource attribute 
    has a value specified in the ZCL.
11. ZED unicasts a ZCL write attributes command frame to write the HWVersion attribute to ZR. ZR unicasts a ZCL
    write attributes response  command frame to ZED with a status of 0x88 (READ_ONLY).



To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
