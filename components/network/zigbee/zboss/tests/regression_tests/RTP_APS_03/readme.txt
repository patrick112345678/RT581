R22 - Light switch and several bulb devices
RTP_APS_03 - light switch joins to network with several bulb devices and send on/off commands consecutively via binding

Objective:

    To confirm that device can returns correct ret codes on exceeding binding transition table capacity then clear table and send new request successfully.

Devices:

       1. TH - ZC
       2. DUT - ZED (light switch)
       3. TH - ZR1 (bulb)
       4. TH - ZR2 (bulb)

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. Max children count is set to 0 for TH ZR1, TH ZR2 (to ensure that DUT ZED will join to network as TH ZC child)

Test procedure:

       1. Power on TH ZC
       2. Power on TH ZR1, TH ZR2
       3. Power on DUT ZED
       4. Wait for TH ZR1, TH ZR2 associate with TH ZC
       5. Wait for DUT ZED associate with TH ZC
       6.1. Wait for DUT ZED send Match Descriptor Request
       6.2. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
       7. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
       8. Wait for DUT ZED sends ZCL On/Off On command equal to trans binding table size.
       9. Wait for DUT ZED sends ZCL On/Off On command after timeout.

Expected outcome:

       1. TH ZC creates a network

       2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC
       3. DUT ZED trace log now contains msg:
          "Trans binding table size is #"

       4.1. DUT ZED sends Match Descriptor Request:
              Input Cluster Count: 2
              Input Cluster List
                     Input Cluster: 0x0006 (On/Off)
                     Input Cluster: 0x0008 (Level Control)
              Output Cluster Count: 0
       4.2. DUT ZED receives Match Descriptor Response from each of bulbs (TH ZR1, TH ZR2, TH ZR3, TH ZR4):
              Endpoint Count: 1
              Matching Endpoint List
                     Endpoint: 10

       5. DUT ZED sends Extended Address Request to each bulb and receive Extended Address Response

       6. DUT ZED sends ZCL On/Off On command and receives mac ack trans table size times.
          (Frame may not be sent to all devices in case of small cb queue size)

       7. In DUT ZED log shall appear:
          [APP1] RET_TABLE_FULL - test OK

       8. After step 7 in DUT trace shall appear:
          [APP1] Table is empty, check that messages can be send

       9. DUT ZED sends ZCL On/Off Off command and receives mac ack once.

       10. After step 7 in DUT trace shall appear:
          [APP1] >> light_control_send_on_off, param = #
       10.1 For param there shall be following trace with param corresponding to param value from step 9.
          [APP1] >> light_control_send_on_off_cb param # status #
          [APP1] RET_OK - successfully got response
