TP-CST-TC-03: Group addressed commands with server as DUT

Please note that for the server the group configuration tests in 3.11 are also required to be run.
Verify that the devices in a Group can receive and handle broadcast messages. Broadcast messages are
typically transmitted by a remote control device in order to control a number of lamp devices at once.
Note that a different group ID to that indicated may be substituted if desired, but it must then be used
throughout the test.


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

1.  ZED sends a "remove all groups" command to zR.
2.  ZED sends an add group command to add ZR to group 1.
3.  ZED broadcasts a ZCL "on" command frame to group ID 0x0001. ZR should turn on.
4.  ZED broadcasts a ZCL "off" command frame to group ID 0x0001. ZR should turn off.
5.  ZED broadcasts a ZCL "toggle" command frame to group ID 0x0001. ZR should turn on.
6.  ZED unicasts a ZCL "read attributes" command frame for the OnOff attribute to ZR.
    ZR unicasts a ZCL "read attributes" response command frame to ZED. OnOff attribute has
    the value 0x01.
7.  ZED broadcasts a ZCL "toggle" command frame to group ID 0x0001. ZR should turn off.
8.  ZED unicasts a ZCL "read attributes" command frame for the OnOff attribute to ZR.
    ZR unicasts a ZCL "read attributes" response command frame to ZED. OnOff attribute has
    the value 0x00.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
