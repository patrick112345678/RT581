Dimmable Light Touchlink sample
-----------------------------------------------------------------------------------------------
This sample contains 2 TL applications - router and end device.
Both devices supports Identify, On/Off and Level Control clusters.

Use case:
1. Flash light_device_zr and light_controller_zed applications on 2 devices.
2. Power on ZR, then wait for 5-10 seconds and power on ZED. Check sniffer logs for Scan Request -
Scan Response and successful Touchlink. On the first join ZED will do Finding and Binding operations
(Active Endpoint and Simple Descriptor requests).
3. ZED should send On/Off Toggle, and then Level Control Move To Level commands to ZR every 7 sec.


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZR - light_device_zr
   2. ZED - light_controller_zed

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   Item 1. ZR start.
   Item 2. ZED start.
   Item 5. Turn ZED off.
   Item 6. Turn ZED on.
   Item 9. Turn ZR off.
   Item 10. Turn ZR on.

 Expected outcome:
    For 'Test procedure' item 2:
      2.1. ZED -> Broadcast: Scan Request
      2.2. ZR -> Broadcast: Scan Response
      2.3. ZED -> ZR: Network Start Request
      2.4. ZR -> ZED: Network Start Response
      2.5 ZED -> ZR: Rejoin
    For 'Test procedure' item 6:
      6.1. ZED -> ZR: Rejoin
    For 'Test procedure' item 10:
      10.1. ZR > Broadcast: Parent Announce
      10.2. ZED -> ZR: Move to Level 0
      10.3. ZED -> ZR: MOVE TO LEVEL 200
