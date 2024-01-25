Test Color Loop Set command

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
8.  ZED send On command.
9.  ZED send Remove all groups command.
10. ZED unicasts a ZCL Move to Color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to 0x000a (1s).

Test steps
1   ZED unicasts a ZLL Color Loop Set command frame to ZR with the update flags field set to 0x0F
    (update direction, time and start hue, action), the action field set to 0x01, the direction field set to 0x01,
    the time field set to 0x000a (10s) and the start hue field set to 0x0000.
2   After 30 sec ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time field set
    to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue
    (red).

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
