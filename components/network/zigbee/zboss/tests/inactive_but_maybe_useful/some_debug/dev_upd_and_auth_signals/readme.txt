This test is based on the Light Sample.

  ZC
   |
  ZR    wait 60 seconds to verify DeviceUpdate/DeviceAuthorized
   |    signals after Secured/Unsecured rejoin for legacy device
   |
  ZED   wait 80 seconds

   Test steps:
0) ZR should be join to ZC, ZED should be join to ZR
   - verify the 'Device Update' signals with 'Unsecured Join' status:
     internal in ZC when ZR is joined;
     internal in ZR when ZED is joined;
     external in ZC when ZED is joined to ZR;
   - verify the 'Device Authorized' signal in ZC when ZED will
     be authorized (after sending 'Verify Key' command);

1) ZR send 'Leave' command to ZED ('Rejoin flag' == FALSE)
   - verify the 'Device Update' signals with 'Device Left' status:
     internal in ZR;
     external in ZC;

2) ZED joins again to ZR
   - verify the 'Device Update' signals with 'Unsecured Join' status:
     internal in ZR when ZED is joined;
     external in ZC when ZED is joined to ZR;
   - verify the 'Device Authorized' signal in ZC when ZED will
     be authorized (after sending 'Verify Key' command);

3) ZR send 'Leave' command to ZED ('Rejoin flag' == TRUE) and
   ZED does rejoin to ZR
   - verify the 'Device Update' signals with 'Secured Rejoin' status
     internal in ZR
     external in ZC

4) ZED leaves from network and rejons to ZR
   - verifivy the 'Device Update' signals with 'Unsecured Rejoin' status:
     internal in ZR
     external in ZC
/* **************************************************************** */


Demo setup:
===========
The demo should be set in the following way:
1. Flash each device individually and power-off after programming.
2. Power on Light Coordinator.
3. Power on Dimmable Light device.
4. Power on Light Control device.

For the detailed information of the each device, refer to the sections below.

Light Coordinator application:
================================
Light Coordinator device establishes and joins devices to the Zigbee network.
LED1 indicates the programm start.

Dimmable Light application:
===========================
Dimmable Light joins the ZC and waits for a Light Control to join the network.
Both On/Off and Level Control clusters are supported.
Device state and brightness level are store in Non-Volatile Memory, so bulb's parameters are restored after power cycle.
LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).
LED3 indicates the bulb state. Lights when On/Off cluster On command received.


Light Control application:
===========================
Light Control joins the ZC/ZR and searches any device with On/Off or Level Control cluster.

LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).

The device has two buttons:
- the "Button 1"  sends On command,
- the "Button 2"  sends OFF command,

If HW does not support buttons - once Dimmable Light is discovered, Light Control starts periodical On-Off-On-Off-...
command sending with 15 sec timeout.
