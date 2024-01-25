R22 - Light switch and several bulb devices
RTP_APS_04 - light switch joins to network with several bulb devices and send on/off commands consecutively via binding

Objective:

    To confirm that device can returns correct ret codes on no bound devices.

Devices:

       1. TH - ZC
       2. DUT - ZED (light switch)
       3. TH - ZR

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. DUT ZED and TH ZR shall join to network as TH ZC child.


Test procedure:

       1. Power on TH ZC
       2. Power on DUT ZED
       3. Wait for DUT ZED associate with TH ZC
       4. Power off DUT ZED
       5. Power on TH ZR
       6. Wait for TH ZR associate with TH ZC
       7. Power on DUT ZED
       8. Wait for DUT ZED associate with TH ZC


Expected outcome:

       1. TH ZC creates a network

       2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. DUT ZED sends match descriptor request and shall got no responses

       4. DUT ZED doesn't send ZCL On/Off On command and receives default response

       5. In DUT ZED log shall appear:
          [APP1] ZB_APS_STATUS_NO_BOUND_DEVICE - test OK

       6. After DUT ZED powered off TH ZR shall be started

       7. TH ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

       8. After successful authorization DUT ZED shall be powered on

       9. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       10. DUT ZED sends match descriptor request and got response from TH ZR

       11. DUT ZED sends on off on command

       12. DUT ZED shall get default response.

       13. There shall be new trace message for DUT ZED:
          [APP2] >> light_control_send_on_off_cb

       14. There shall be no new trace message for DUT ZED:
          [APP1] ZB_APS_STATUS_NO_BOUND_DEVICE - test OK
