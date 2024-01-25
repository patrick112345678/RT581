TP-CST-TC-04: Level control cluster with server as DUT

Verify performance of the Server Level Control Cluster. This cluster is typically used by a remote
control device to make a lamp device more or less bright.
NOTE: the resolution of the server’s level setting ability need not be exact. When verifying that a
server set a level to a target, the DUT’s CurrentLevel attribute need only be within +/-5 of the expected
outcome unless otherwise specified. Note that this test specification places no requirements at all on
any physical changes being observed or measured.

Note that if the device is not able to move at a variable rate then the transition time and rate fields of
the move and move to level commands (and their “with On/off” equivalents) may be ignored and a
fixed rate used instead.


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

8.  ZED unicasts a ZCL on command frame to ZR. ZR should turn on.
9.  ZED unicasts a ZCL move to level (with on/off) command frame to ZR, with the level 
    field set 0x80. ZR should remain on.
10. ZED unicasts a ZCL off command frame to ZR. ZR should turn off.


Test steps

1.  ZED unicasts a ZCL read attributes command frame for the CurrentLevel attribute 
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. 
    CurrentLevel attribute has the value 0x80.
2.  ZED unicasts a ZCL move (with on/off) command to ZR, with the move mode field set 
    to 0x00 (move up) and the rate field set to 0x40. ZR turns on and increases to 
    full brightness over 4 seconds.
3.  ZED unicasts a ZCL read attributes command frame for the CurrentLevel attribute 
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0xfe.
    Note: CurrentLevel attribute has the value 0xff. See ZCL spec 3.10.2.2.
4.  ZED unicasts a ZCL move to level command to ZR, with the level field set
    of 0x80 and the transition time field set to 0x0000. ZR immediately reduces its 
    brightness to its mid point.
5.  ZED unicasts a ZCL read attributes command frame for the CurrentLevel attribute 
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x80.
6.  ZED unicasts a ZCL off command to ZR. ZR turns off.
7.  ZED unicasts a ZCL read attributes command frame for the CurrentLevel attribute 
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x80.
8.  ZED unicasts a ZCL on command to ZR. ZR turns on.
9.  ZED unicasts a ZCL read attributes command frame for the CurrentLevel attribute
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x80.
10. ZED unicasts a ZCL step command to ZR, with the step mode field set to 0x01
    (step down), the step size field set to 0x40 and the transition time field set 
    to 0x0014 (2s). ZR reduces its brightness.
11. Wait 2 sec
12. ZED unicasts a ZCL step command to ZR, with the step mode field set to 0x01
    (step down), the step size field set to 0x40 and the transition time field set 
    to 0x0014 (2s). ZR reduces its brightness to its minimum level.
11. Wait 2 sec
13. ZED unicasts a ZCL read attributes command for the CurrentLevel attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. CurrentLevel 
    attribute has the value 0x01.
    Note: CurrentLevel attribute has the value 0x00. See ZCL spec 3.10.2.2.
14. ZED unicasts a ZCL toggle command to ZR. ZR turns off.
15. ZED unicasts a ZCL read attributes command for the CurrentLevel and OnOff
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x01. OnOff attribute has the value 0x00.
16. ZED unicasts a ZCL toggle command to ZR. ZR turns on.
17. ZED unicasts a ZCL read attributes command for the CurrentLevel and OnOff
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x01. OnOff attribute has the value 0x01.
18. ZED unicasts a ZCL move command to ZR, with the move mode field set to 0x00 
    (move up) and the rate field set to 0x0a. ZR begins to increase its brightness.
19. Wait 10 sec
20. ZED unicasts a ZCL stop [0x03] command to ZR. ZR stops adjusting its brightness.
21. ZED unicasts a ZCL read attributes command for the CurrentLevel attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. CurrentLevel 
    attribute has the value 0x65.
22. ZED unicasts a ZCL move command to ZR, with the move mode field set to 0x01 
    (move down) and the rate field set to 0x04. ZR begins to decrease its brightness.
23. Wait 10 sec
24. ZED unicasts a ZCL stop [0x07] command to ZR. ZR stops adjusting its brightness.
25. ZED unicasts a ZCL read attributes command for the CurrentLevel attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. CurrentLevel 
    attribute has the value 0x3d.
26. Wait 60 sec.
27. ZED unicasts a ZCL move to level (with on/off) command to ZR, with the level field 
    set to 0x00 and the transition time field set to 0x0258 (60s). ZR “light”
    immediately begins to decrease its brightness.
28. Wait 10 sec.
29. ZED unicasts a ZCL read attributes command for the RemainingTime attribute to ZR.
    ZR unicasts a ZCL read attributes response command frame to ZED. RemainingTime 
    attribute has a value approximately equal to 0x001f4. ZR decreases its brightness 
    to its minimum level and turns off.
30. Wait 60 sec
31. ZED unicasts a ZCL read attributes command for the CurrentLevel and OnOff attributes 
    to ZR. ZR unicasts a ZCL read attributes response command frame to ZED. CurrentLevel 
    attribute has the value 0x01. OnOff attribute has the value 0x00.
32. ZED unicasts a ZCL step (with on/off) command to ZR, with the step mode field set 
    to 0x00 (step up), the step size field set to 0x01 and the transition time field 
    set to 0xffff (as fast as possible). ZR turns on at its minimum brightness.
33. ZED unicasts a ZCL read attributes command for the CurrentLevel and OnOff 
    attributes to ZR. ZR unicasts a ZCL read attributes response command frame to ZED.
    CurrentLevel attribute has the value 0x02. OnOff attribute has the value 0x01.
    Note: CurrentLevel attribute has the value 0x01 (min value "0" + 1). See ZCL spec 3.10.2.2.

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
