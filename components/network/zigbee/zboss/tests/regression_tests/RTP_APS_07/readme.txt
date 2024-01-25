R22 - Light switch and several bulb devices
RTP_APS_07 - light switch joins to network with several bulb devices and send commands with short interval then reboots and send them again

Objective:

  To confirm that after sending requests by bindings locked addresses correctly unlocked.

Devices:

       1. TH - ZC
       2. DUT - ZED (light switch)
       3. TH - ZR1 (bulb)
       4. TH - ZR2 (bulb)
       5. TH - ZR3 (bulb)
       6. TH - ZR4 (bulb)

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. Max children count is set to 0 for TH ZR1, TH ZR2, TH ZR3, TH ZR4 (to ensure that DUT ZED will join to network as TH ZC child)

Test procedure:

       1. Power on TH ZC
       2. Power on TH ZR1, TH ZR2, TH ZR3, TH ZR4
       3. Power on DUT ZED
       4. Wait for TH ZR1, TH ZR2, TH ZR3, TH ZR4 associate with TH ZC
       5. Wait for DUT ZED associate with TH ZC
       6.1. Wait for DUT ZED send Match Descriptor Request
       6.2. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
       7. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
       8.1. DUT ZED sends ZCL On/Off On command
       8.2. DUT ZED sends ZCL On/Off Off command
       9. Reboot DUT ZED without erasing nvram
       10. Wait for DUT ZED send Match Descriptor Request
       11. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
       12. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
       13.1. DUT ZED sends ZCL On/Off On command
       13.2. DUT ZED sends ZCL On/Off Off command
       16. Repeat step 9-13 3 times.

Expected outcome:

       1. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       2. In DUT ZED trace logs shall be no msgs with:
             "TEST ERROR"

       3.1. DUT ZED sends ZCL On/Off On command to any router
       3.2. DUT ZED sends ZCL On/Off Off command to any router

       4.1. DUT ZED sends ZCL On/Off On command to any router
       4.2. DUT ZED sends ZCL On/Off Off command to any router
