TP-CST-TC-17: Color control cluster (color loop) with server as DUT

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
10. ZED unicasts a ZCL Move to Color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to 0x000a (1s).

Test steps
1a  ZED unicasts a ZLL Color Loop Set command frame to ZR with the update flags field set to 0x0e
    (update direction, time and start hue), the action field set to 0x00, the direction field set to 0x00,
    the time field set to 0x000a (10s) and the start hue field set to 0x0000.
1b  ZED unicasts a ZCL read attributes command frame for the ColorLoopActive, ColorLoopDirection,
    ColorLoopTime and ColorLoopStartEnhancedHue attributes to ZR. ZR unicasts a ZCL read
    attributes response command frame to ZED. ColorLoopActive attribute has the value 0x00.
    ColorLoopDirection attribute has the value 0x00. ColorLoopTime attribute has the value 0x000a.
    ColorLoopStartEnhancedHue attribute has the value 0x0000.
2a  ZED unicasts a ZLL Color Loop Set command frame to ZR with the update flags field
    set to 0x02 (update direction), the action field set to 0x01, the direction field set to 0x01,
    the time field set to 0x0000 and the start hue field set to 0x0000.
2b  ZED unicasts a ZCL read attributes command frame for the ColorLoopActive, ColorLoopDirection,
    ColorLoopTime and ColorLoopStartEnhancedHue attributes to ZR. ZR unicasts a ZCL read
    attributes response command frame to ZED. ColorLoopActive attribute has the value 0x00.
    ColorLoopDirection attribute has the value 0x01. ColorLoopTime attribute has the value 0x000a.
    ColorLoopStartEnhancedHue attribute has the value 0x0000.
3a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x01 (update action), the action field set to 0x02 (start from enhanced current hue),
    the direction field set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000.
    ZR begins a color loop cycle (starting from the current enhanced hue) with a color cycle
    time of 10s.
3b  ZED unicasts a ZCL read attributes command frame for the ColorLoopActive attribute to DUT
    SERVER. ZR unicasts a ZCL read attributes response command frame to ZED. ColorLoopActive 
    attribute has the value 0x01.
3c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field
    set to 0x01 (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00,
    the time field set to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop
    cycle and returns to its previous hue (red).
3d  ZED unicasts a ZCL read attributes command frame for the ColorLoopActive attribute to DUT
    SERVER. ZR unicasts a ZCL read attributes response command frame to ZED. ColorLoopActive
    attribute has the value 0x00.
4a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x01 (start from ColorLoopStartEnhancedHue), the direction field
    set to 0x00, the time field set to 0x0000 and the start hue field set to 0x0000. ZR begins a color loop
    cycle (not starting from the current enhanced hue) with a color cycle time of 10s.
4b  ZED unicasts a ZCL read attributes command frame for the ColorLoopStoredEnhancedHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
    attribute has a value commensurate with a red hue.
4c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time field set
    to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue
    (red).
5a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x02 (start from enhanced current hue), the direction field set to
    0x00, the time field set to 0x0000 and the start hue field set to 0x0000. ZR begins a color loop
    cycle (starting from the current enhanced hue) with a color cycle time of 10s.
5b  ZED unicasts a ZCL read attributes command frame for the ColorLoopStoredEnhancedHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
    attribute has a value commensurate with a red hue.
5c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
     (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time
    field set to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to
    its previous hue (red).
6a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x05
    (update action and time), the action field set to 0x02 (start from enhanced current hue), the direction field
    set to 0x00, the time field set to 0x0064 (100s) and the start hue field set to 0x0000. ZR begins a
    Color Loop Cycle (starting from the current enhanced hue) with a color cycle time of 100s.
6b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x02
    (update direction), the action field set to 0x00, the direction field set to 0x00, the time field set to
    0x0000 and the start hue field set to 0x0000. ZR reverses the direction of the color loop cycle but retains the
    color cycle time of 100s.
6c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time field set to
    0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue
    (red).
7a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x07
    (update action, direction and time), the action field set to 0x02 (start from enhanced current hue), the
    direction field set to 0x01 (increment), the time field set to 0x0064 (100s) and the start hue field set to 0x0000.
    ZR begins a color loop cycle (starting from the current enhanced hue) with a color cycle time of 100s.
7b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x04
    (update time), the action field set to 0x00, the direction field set to 0x00, the time field set to 0x000a (10s)
    and the start hue field set to 0x0000. ZR adapts its color loop cycle time to 10s.
7c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time field set
    to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue
    (red).
8a  ZED unicasts a ZCL read attributes command frame for the EnhancedCurrentHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. EnhancedCurrentHue attribute has a
    value commensurate with a red hue.
8b  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x05
    (update action and time), the action field set to 0x02 (start from enhanced current hue), the direction field
    set to 0x00, the time field set to 0x001e (30s) and the start hue field set to 0x0000. ZR begins a color loop
    cycle (starting from the current enhanced hue) with a color cycle time of 30s.
8c  ZED unicasts a ZCL read attributes command frame for the ColorLoopStoredEnhancedHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
    attribute has a value equal to the value of the EnhancedCurrentHue attribute read in step 8.
8d  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01 (update action), the action
    field set to 0x00 (deactivate), the direction field set to 0x00, the time field set to 0x0000 and the start hue
    field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue (red).
8e  ZED unicasts a ZCL read attributes command frame for the EnhancedCurrentHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. EnhancedCurrentHue attribute has a
    value equal to the value of the ColorLoopStoredEnhancedHue attribute read in step 8b.
9a  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x07
    (update action, direction and time), the action field set to 0x02 (start from enhanced current hue), the
    direction field set to 0x01 (increment), the time field set to 0x000a (10s) and the start hue field
    set to 0x0000. ZR begins a color loop cycle (starting from the current enhanced hue) with a color cycle
    time of 10s.
9b  ZED unicasts a ZLL enhanced move to hue command frame to ZR with the enhanced hue field set to an
    appropriate value for a blue hue, the direction field set to 0x00 (shortest distance) and the transition time field
    set to 0x000a (1s). ZR may change to a blue hue but the color loop continues.
9c  ZED unicasts a ZLL color loop set command frame to ZR with the update flags field set to 0x01
    (update action), the action field set to 0x00 (deactivate), the direction field set to 0x00, the time field set
    to 0x0000 and the start hue field set to 0x0000. ZR stops the color loop cycle and returns to its previous hue
    (red).

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
