TP-CST-TC-22: Color control cluster (color temperature) with client as DUT

This test applies to the color loop mechanism in the color control cluster.

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
10. ZED unicasts a ZCL Move to Color command frame to ZR with the ColorX and ColorY
    fields set to appropriate values for a red color and the transition time field
    set to 0x000a (1s).

Test steps
1   ZED unicasts a ZCL move to color temperature command frame to ZR with the color
    temperature field and the transition time field set to appropriate values. ZR
    transitions to the requested color temperature over the requested time.
2a  ZED unicasts a ZLL move color temperature command frame to ZR with the move mode
    field set to 0x01 (move up) and all other fields set to appropriate values. ZR 
    begins to increment its color temperature.
2b  After 10s ZED unicasts a ZLL move color temperature command frame to ZR with the
    move mode field set to 0x00 (stop the move), the rate field set to 0x0000, the
    color temperature minimum field set to 0x0000 and the color temperature maximum
    field set to 0x0000. ZR stops incrementing its color temperature.
3a  ZED unicasts a ZLL move color temperature command frame to ZR with the move mode
    field set to 0x03 (move down) and all other fields set to appropriate values.
    ZR begins to decrement its color temperature.
3b  After 10s ZED unicasts a ZLL move color temperature command frame to ZR with the
    move mode field set to 0x00 (stop the move), the rate field set to 0x0000, the
    color temperature minimum field set to 0x0000 and the color temperature maximum
    field set to 0x0000. ZR stops decrementing its color temperature.
4a  ZED unicasts a ZLL step color temperature command frame to ZR with the step mode
    field set to 0x01 (step up) and all other fields set to appropriate values. ZR
    begins to increment its color temperature.
4b  After 10s ZED unicasts a ZLL stop move step command frame to ZR. ZR stops
    incrementing its color temperature.
5a  ZED unicasts a ZLL step color temperature command frame to ZR with the step mode
    field set to 0x03 (step down) and all other fields set to appropriate values. ZR
    begins to decrement its color temperature.
5b  After 10s ZED unicasts a ZLL stop move step command frame to ZR. ZR stops
    decrementing its color temperature.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
