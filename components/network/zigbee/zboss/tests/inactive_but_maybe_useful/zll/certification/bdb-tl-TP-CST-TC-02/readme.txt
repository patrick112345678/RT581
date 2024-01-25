TP-CST-TC-02: On/off cluster with client as DUT

Verify functionality of client on/off cluster. This cluster is typically used by a remote control device to
turn a lamp device on or off.



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
1.  ZED send unicasts a ZCL "on" command to gZR.
2.  ZED send unicasts a ZCL "off" command to gZR.
3.  ZED send unicasts a ZCL "toggle" command to gZR.
4.  ZED send unicasts a ZCL "read attributes" command for the OnOff attribute to gZR.
    gZR send unicasts a ZCL "read attributes" response to ZED. OnOff attribute has the value 0x01.
5.  ZED send unicasts a ZCL "off" or "toggle" command to gZR.
6.  ZED send unicasts a ZCL "read attributes" command for the OnOff attribute to gZR.
    gZR send unicasts a ZCL "read attributes" response to ZED. OnOff attribute has the value 0x00.
7.  ZED send unicasts a ZLL "off with effect" command to gZR, with the "effect identifier" and 
    "effect variant" fields set to non-reserved values.
8.  ZED send unicasts a ZLL "on with recall global scene" command to gZR.
9.  ZED send unicasts a ZLL "on with timed off" command to gZR.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
