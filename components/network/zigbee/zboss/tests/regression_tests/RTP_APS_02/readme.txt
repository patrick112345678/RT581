R22 - Light switch and several bulb devices
RTP_APS_02 - light switch joins to network with several bulb devices and send on/off commands consecutively via binding

Objective:

    To confirm that device can send multiple messages via binding and perform all necessary action to clean tables after successful sending.

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
       3. TH ZR1, TH ZR2, TH ZR3, TH ZR4 associates with TH ZC
       4. Power on DUT ZED
       5. DUT ZED associates with TH ZC
       6. DUT ZED sends Match Descriptor Request and receives Match Descriptor Response from each of bulbs
       7. DUT ZED sends Extended Address Request to each bulb and receives Extended Address Response
       8. DUT ZED sends ZCL On/Off On command to all bound devices 2 times

Expected outcome:

       1. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       2. DUT ZED sends ZCL On/Off On command to all bound devices

       3.1. In DUT ZED trace shall be:
               [APP1] >> light_control_send_on_off, param = #
       3.2. For all parameter id from this trace shall be returned response with same buffer id:
               [APP1] >> light_control_send_on_off_cb, param = #

       4. Repeat steps 2 and 3.

    5. Following trace message shall appear:
               [ERROR] >> test_errors_manager

    6. Following trace message shall not appear:
               [ERROR] test_errors_manager: TEST FAILED
