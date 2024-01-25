TP-CST-TC-18: Color control cluster (color loop) with client as DUT

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

Test steps
1a  ZED unicasts a ZLL color loop set command frame to ZR with the action field set
    to 0x02 (start from enhanced current hue) and all other fields set to appropriate
    values. ZR begins a color loop cycle (starting from the current enhanced hue).
1b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x01 (update action), the action field set to 0x00 (deactivate), the direction
    field set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000.
    ZR stops the color loop cycle and returns to its previous hue (red).
2a  ZED unicasts a ZLL color loop set command frame to ZR with the action field set to
    0x01 (start from ColorLoopStartEnhancedHue) and all other fields set to appropriate
    values. ZR begins a color loop cycle (not starting from the current enhanced hue).
2b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x01 (update action), the action field set to 0x00 (deactivate), the direction
    field set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000.
    ZR stops the color loop cycle and returns to its previous hue (red).
3a  ZED unicasts a ZLL color loop set command frame to ZR with the action field set to
    0x02 (start from enhanced current hue) and all other fields set to appropriate values.
    ZR begins a color loop cycle (starting from the current enhanced hue).
3b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x02 (update direction), the action field set to 0x00, the direction field set
    to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000.
    ZR reverses the direction of the color loop cycle.
3c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x01 (update action), the action field set to 0x00 (deactivate), the direction 
    field set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000.
    ZR stops the color loop cycle and returns to its previous hue (red).
4a  ZED unicasts a ZLL color loop set command frame to ZR with the action field set to
    0x02 (start from enhanced current hue) and all other fields set to appropriate values.
    ZR begins a color loop cycle (starting from the current enhanced hue).
4b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x04 (update time), the action field set to 0x00, the direction field set to 0x00,
    the time field set to an appropriate value, different to when the color loop was started,
    and the start hue field set to 0x0000. ZR adapts its color loop cycle time.
4c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set
    to 0x01 (update action), the action field set to 0x00 (deactivate), the direction field
    set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000. ZR stops
    the color loop cycle and returns to its previous hue (red).

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
