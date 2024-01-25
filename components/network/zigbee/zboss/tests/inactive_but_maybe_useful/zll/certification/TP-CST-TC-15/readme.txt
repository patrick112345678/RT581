TP-CST-TC-15: Color control cluster with server as DUT

This cluster is typically used for setting or querying the color of a colored lamp.
When verifying the physical change in color by observation, deviation from the specified color is
permitted. Additionally, implementations are not required to implement the full color range. Where a
color indicated in this test specification is not achievable by the DUT, the nearest possible alternative
color should be substituted.



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
1.  ZED unicasts a ZCL read attributes command frame for the ColorCapabilities attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. ColorCapabilities attribute has a
    value commensurate with the device type:
    ADCL7:  0x000f
    ADECL7: 0x001f
    ADCTL7: 0x0010
2.  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to 0x0032 (5s).
    ZR changes to a red color.
3a  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a green color and the transition time field set to 0x00c8 (20s).
    ZR begins to changes to a green color.
3b  ZED unicasts a ZCL read attributes command frame for the RemainingTime attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. RemainingTime attribute has a
    value approximately equal to 0x0064.
3c  ZED unicasts a ZCL read attributes command frame for the RemainingTime attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. RemainingTime attribute has the
    value 0x0000. ZR has changed to a green color.
4   ZED unicasts a ZCL read attributes command frame for the CurrentX and CurrentY attributes to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. CurrentX and CurrentY attributes
    have values corresponding to a green color.
5   ZED unicasts a ZCL read attributes command frame for the ColorMode attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. ColorMode attribute has the value 0x01 (CurrentX and CurrentY).
6   ZED unicasts a ZCL read attributes command frame for the NumberOfPrimaries attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. NumberOfPrimaries attribute has
    a value between 0x01 and 0x06.
6a  ZED unicasts a ZCL read attributes command frame for the Primary1X, Primary1Y and Primary1Intensity
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary1X, Primary1Y
    and Primary1Intensity attributes have values appropriate for primary 1.
6b  ZED unicasts a ZCL read attributes command frame for the Primary2X, Primary2Y and Primary2Intensity
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary2X, Primary2Y
    and Primary2Intensity attributes have values appropriate for primary 2.
6c  ZED unicasts a ZCL read attributes command frame for the Primary3X, Primary3Y and Primary3Intensity 
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary3X, Primary3Y
    and Primary3Intensity attributes have values appropriate for primary 3.
6d  ZED unicasts a ZCL read attributes command frame for the Primary4X, Primary4Y and Primary4Intensity 
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary4X, Primary4Y
    and Primary4Intensity attributes have values appropriate for primary 4.
6e  ZED unicasts a ZCL read attributes command frame for the Primary5X, Primary5Y and Primary5Intensity
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary5X, Primary5Y
    and Primary5Intensity attributes have values appropriate for primary 5.
6f  ZED unicasts a ZCL read attributes command frame for the Primary6X, Primary6Y and Primary6Intensity 
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. Primary6X, Primary6Y
    and Primary6Intensity attributes have values appropriate for primary 6.
7   ZED unicasts a ZCL read attributes command frame for the EnhancedColorMode attribute to ZR. ZR unicasts 
    a ZCL read attributes response command frame to ZED. EnhancedColorMode attribute has the value 0x01 
    (CurrentX and CurrentY).
8   ZED unicasts a ZCL move to hue command frame to ZR with the hue field set to an appropriate value for a
    blue hue, the direction field set to 0x00 (shortest distance) and the transition time field set to 
    0x0032 (5s). ZR changes to a blue color.
9   ZED unicasts a ZCL read attributes command frame for the ColorMode and EnhancedColorMode attributes to
    ZR. ZR unicasts a ZCL read attributes response command frame to ZED. ColorMode and EnhancedColorMode 
    attributes both have the value 0x00 (CurrentHue and CurrentSaturation).
10  ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x01 (move up) and the
    rate field set to 0x0a (10). ZR changes hue in an upward direction.
11  ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x03 (move down) and 
    the rate field set to 0x14 (20). ZR changes hue in a downward direction.
12  ZED unicasts a ZCL move hue command frame to ZR with the move mode field set to 0x00 (stop). ZR stops
    changing hue.
13  ZED unicasts a ZCL step hue command frame to ZR with the step mode field set to 0x01 (step up), the
    step size field set to 0x40 (64) and the transition time field set to 0x64 (100). ZR steps hue in an
    upward direction.
14  ZED unicasts a ZCL move to saturation command frame to ZR with the saturation field set to 0x00
    (white) and the transition time field set to 0x0032 (5s). ZR reduces its saturation down to 0 (white).
15  ZED unicasts a ZCL move saturation command frame to ZR with the move mode field set to 0x01 (move up)
    and the rate field set to 0x0a (10). ZR changes saturation in an upward direction.
16  ZED unicasts a ZCL move saturation command frame to ZR with the move mode field set to 0x03 (move down) 
    and the rate field set to 0x14 (20). ZR changes saturation in a downward direction.
    NOTE test description has error: Test Step not correspondent Verification.
17  ZED unicasts a ZCL move saturation command frame to ZR with the move mode field set to 0x00 (stop).
    ZR stops changing saturation.
18  ZED unicasts a ZCL step saturation command frame to ZR with the step mode field set to 0x01 (step up),
    the step size field set to 0x40 (64) and the transition time field set to 0x64 (100). ZR steps 
    saturation in an upward direction.
19a ZED unicasts a ZCL move to hue and saturation command frame to ZR with the hue field set to
    an appropriate value for a blue hue, the saturation field set to 0x7f (50%) and the transition time
    field set to 0x0064 (10s). ZR changes to a blue hue at 50% saturation.
19b ZED unicasts a ZCL read attributes command frame for the CurrentHue attribute to ZR. ZR unicasts a ZCL
    read attributes response command frame to ZED. CurrentHue attribute has a value commensurate with 
    a blue hue.
19c ZED unicasts a ZCL read attributes command frame for the CurrentSaturation attribute to ZR. ZR 
    unicasts a ZCL read attributes response command frame to ZED. CurrentSaturation attribute has the 
    value 0x80.
20a ZED unicasts a ZCL store scene command frame to ZR with the group ID field set to 0x0000 and the scene
    ID field set to 0x01. ZR unicasts a ZCL store scene response command frame to ZED with the status field
    set to 0x00 (SUCCESS), the group ID field set to 0x0000 and the scene ID field set to 0x01.
20b ZED unicasts a ZCL read attributes command frame for the CurrentX, CurrentY, EnhancedCurrentHue and
    CurrentSaturation attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    Store the values of the CurrentX, CurrentY, EnhancedCurrentHue and CurrentSaturation attributes.
20c ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a red color and the transition time field set to 0x0032 (5s). ZR changes to 
    a red color.
20d ZED unicasts a ZCL recall scene command frame to ZR with the group ID field set to 0x0000 and the 
    scene ID field set to 0x01. ZR changes to a blue color.
20e ZED unicasts a ZCL read attributes command frame for the CurrentX, CurrentY, EnhancedCurrentHue and
    CurrentSaturation attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    Verify that the values of the CurrentX, CurrentY, EnhancedCurrentHue and CurrentSaturation attributes
    are the same as was stored previously in step 20a.
21  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for a green color and the transition time field set to 0x00c8 (20s). ZR changes
    to a green color.
22  ZED unicasts a ZCL move color command frame to ZR with the RateX field set to 0x03e8 (+1000) and
    the RateY field set to 0xd8f0 (-10000). ZR begins to change color.
23  ZED unicasts a ZCL move color command frame to ZR with the RateX field set to 0xec78 (-5000) and
    the RateY field set to 0xff9c (-100). ZR begins to change color.
24  ZED unicasts a ZCL move color command frame to ZR with the RateX and RateY fields both set to
    0x0000 (stop). ZR stops changing color.
25  ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for the choice of color and the transition time field set to 0x0032 (5s).
    ZR changes to the color of choice.
26  ZED unicasts a ZCL step color command frame to ZR with the StepX field set to 0x01f4 (+500),
    the StepY field set to 0xfc18 (-1000) and the transition time field set to 0x0064 (10s).
    ZR begins to change color.
27а ZED unicasts a ZLL enhanced move to hue command frame to ZR with the enhanced hue field set to an
    appropriate value for a blue hue, the direction field set to 0x00 (shortest distance) and the
    transition time field set to 0x0032 (5s). ZR changes to a blue hue.
27b ZED unicasts a ZCL read attributes command frame for the EnhancedCurrentHue attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. EnhancedCurrentHue attribute
    has a value commensurate with a blue hue.
28  ZED unicasts a ZLL enhanced move to hue command frame to ZR with the enhanced hue field set to 
    a value for an unobtainable hue, the direction field set to 0x00 (shortest distance) and the
    transition time field set to 0x0032 (5s). ZR unicasts a ZCL default response command frame
    to ZED with the status field set to 0x87 (INVALID_VALUE). ZR does not change color.
29  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x01
    (move up) and the rate field set to 0x0a (10). ZR changes hue in an upward direction.
30  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x03
    (move down) and the rate field set to 0x14 (20). ZR changes hue in a downward direction.
31  ZED unicasts a ZLL enhanced move hue command frame to ZR with the move mode field set to 0x00
    (stop). ZR stops changing hue.
32  ZED unicasts a ZLL enhanced step hue command frame to ZR with the step mode field set to 0x01 
    (step up), the step size field set to 0x4000 (16384) and the transition time field set to 0x0a
    (10). ZR changes hue in an upward direction.
33  ZED unicasts a ZLL enhanced move to hue and saturation command frame to ZR with the enhanced hue
    field set an appropriate value for a blue hue, the saturation field set to 0x7f (50%) and the
    transition time field set to 0x0a (10). ZR changes to a blue hue with 50% saturation over 10s.
    ZED unicasts a ZLL enhanced move to hue and saturation command frame to ZR with the enhanced hue
    field set an appropriate value for a blue hue, the saturation field set to 0x7f (50%) and the
    transition time field set to 0x0a (10). ZR changes to a blue hue with 50% saturation over 10s.
34  ZED unicasts a ZLL enhanced move to hue and saturation command frame to ZR with the enhanced hue
    field set to a value for an unobtainable hue, the saturation field set to 0xfe (maximum
    saturation) and the transition time field set to 0x0032 (5s). ZR unicasts a ZCL default response
    command frame to ZED with the status field set to 0x87 (INVALID_VALUE). ZR does not change color.
35a ZED unicasts a ZCL move to color command frame to ZR with the ColorX and ColorY fields set to
    appropriate values for the choice of color and the transition time field set to 0x0064 (10s).
    ZR begins to change to the color of choice.
35b ZED unicasts a ZLL stop move step command frame to ZR. ZR stops changing color.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
