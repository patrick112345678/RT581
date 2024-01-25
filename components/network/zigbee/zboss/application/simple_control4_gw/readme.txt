Simple Gateway sample
--------------------------------------------------------
This application is a Zigbee Coordinator device which is implemented using BDB API.
Default operational channel is set by ZB_DEFAULT_APS_CHANNEL_MASK.
Maximum operational devices number is 20.

Usage:
Upload a Simple Gateway binary to your Zigbee device. After powering it on the first time,
a Simple GW forms a ZB network. After power restarting the device, existing ZB network
parameters are used, the same network exists (if NVRAM erasing at start is not configured).
On start up the Simple Gateway opens the network for 180 sec. During this time any device joining
is allowed. In order to join your ZB device (bulb/smart plug/etc), power it on during this time
frame - the device will associate the network automatically.

After the successful association, the Simple GW applications starts discovering On/Off Server
cluster. If it is found, Simple GW configures bindings and reporting for the device and
then then it starts to send On/Off Toggle commands periodically. Timeout between
consecutive commands sending is calculated using the following formula:
T = (device_index * 5) sec.
device_index is a number of joined device: 1, 2, ..., 20.

Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - simple_control4_gw
   2. ZED - bulb

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   1. Turn ZC on.
   2. Turn ZED on.
   3. On/Off toggle cmds.

 Expected outcome:
   For 'Test procedure' item 2:
     2.1. Association Request
     2.2. Association Response
     2.3. Transport key
     2.4. Device announcement
     2.5. ZAP request
     2.6. Identify - reports sent to broadcast (0xfffc)
     2.7. Immediate announce
     2.8. Announce - reports sent to ZC
     2.9. ZAP request
   For 'Test procedure' item 3:
     3.1. ZC -> ZED: Toggle
     3.2. ZED -> ZC: Default Response
