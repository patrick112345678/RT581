TP-CST-TC-16: Color control cluster with client as DUT

This cluster is typically used for setting or querying the color of a colored lamp.
When verifying the physical change in color by observation, deviation from the specified color is
permitted. Additionally, implementations are not required to implement the full color range. Where a
color indicated in this test specification is not achievable, the nearest possible alternative color should
be substituted.

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
1   ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to an appropriate time. ZR changes
    to a red color.
2   ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a green color and the transition time field set to an appropriate time. ZR begins
    to changes to a green color.
3   ZED unicasts a ZCL move to hue command frame to ZR with the hue field set to an appropriate value
    for a blue hue, the direction field set to 0x00 (shortest distance) and the transition time field set to an
    appropriate time. ZR changes to a blue color.
4   ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x01 (move up) and
    the rate field set to an appropriate value. ZR changes hue in an upward direction.
5   ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x03 (move down)
    and the rate field set to an appropriate value. ZR changes hue in a downward direction.
6   ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x00 (stop).
    ZR stops changing hue.
7   ZED unicasts a ZCL step hue command frame to ZR with the step mode field set to 0x01 (step up) and
    the step size and transition time fields set to appropriate values. ZR steps hue in an upward direction.
8   ZED unicasts a ZCL move to saturation command frame to ZR with the saturation field set to 0x00
    (white) and the transition time field set to an appropriate value. ZR reduces its saturation down to 0
    (white).
9   ZED unicasts a ZCL move saturation command frame to ZR with the move mode field set to 0x01 (move up)
    and the rate field set to an appropriate value. ZR changes saturation in an upward direction.
10  ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x03 (move down)
    and the rate field set to an appropriate value. ZR changes saturation in a downward direction.
11  ZED unicasts a ZCL move saturation command frame to ZR with the move mode field set to 0x00 (stop).
    ZR stops changing saturation.
12  ZED unicasts a ZCL step saturation command frame to ZR with the step mode field set to 0x01 (step up)
    and the step size and transition time fields set to appropriate values. ZR steps saturation in an upward
    direction.
13  ZED unicasts a ZCL move to hue and saturation command frame to ZR with the hue field set to an
    appropriate value for a blue hue and the saturation and transition time fields set to appropriate values.
    ZR changes to a blue hue at 50% saturation.
14a ZED unicasts a ZCL store scene command frame to ZR with the group ID field set to 0x0000 and the scene
    ID field set to an appropriate value. ZR unicasts a ZCL store scene response command frame to ZED with the
    status field set to 0x00 (SUCCESS), the group ID field set to 0x0000 and the scene ID field set to the scene that was
    stored.
14b ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to an appropriate value. ZR changes to
    a red color.
14c ZED unicasts a ZCL recall scene command frame to ZR with the group ID field set to 0x0000 and the scene
    ID field set to the value used in step 14a. ZR changes to a blue color.
15  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a green color and the transition time field set to an appropriate value. ZR changes to a
    green color.
16  ZED unicasts a ZCL move color command frame to ZR with the RateX and the RateY field set to appropriate
    values. ZR begins to change color.
17  ZED unicasts a ZCL move color command frame to ZR with the RateX and RateY fields both set to 0x0000
    (stop). ZR stops changing color.
18  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to appropriate
    values for the choice of color and the transition time field set to an appropriate value. ZR changes to the
    color of choice.
19  ZED unicasts a ZCL step color command frame to ZR with the StepX, StepY and the transition time fields set
    to appropriate values. ZR begins to change color.
20  ZED unicasts a ZLL enhanced move to hue command frame to ZR with the enhanced hue field set to an
    appropriate value for a blue hue, the direction field set to 0x00 (shortest distance) and the transition time field set
    to an appropriate value. ZR changes to a blue hue.
21  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x01 (move up)
    and the rate field set to an appropriate value. ZR changes hue in an upward direction.
22  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x03 (move down)
    and the rate field set to an appropriate value. ZR changes hue in a downward direction.
23  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x00 (stop).
    ZR stops changing hue.
24  ZED unicasts a ZLL enhanced step hue command frame to ZR with the step mode field set to 0x01 (step
    up), the step size and transition time fields set to appropriate values. ZR changes hue in an upward direction.
25  ZED unicasts a ZLL enhanced move to hue and saturation command frame to ZR with the enhanced hue
    field set an appropriate value for a blue hue and the saturation and transition time fields set to appropriate values.
    ZR changes to a blue hue with a specific saturation.
26a ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to appropriate
    values for the choice of color and the transition time field set to an appropriate value. ZR begins to change to
    the color of choice.
26b ZED unicasts a ZLL stop move step command frame to ZR. ZR stops changing color.

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
