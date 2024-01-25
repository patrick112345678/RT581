R22 - Light switch and several bulb devices
RTP_APS_12 - light switch joins to network with several bulb devices and send commands with short interval

Objective:

       To confirm that the light switch does correctly handles missed aps ACK in case of binding transmissions.

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
       2. Power on TH ZR1, TH ZR2, TH ZR3, TH ZR4
       3. Power on DUT ZED
       4. Wait for TH ZR1, TH ZR2 associate with TH ZC
       5. Wait for DUT ZED associate with TH ZC
       6.1. Wait for DUT ZED send Match Descriptor Request
       6.2. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
       7. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
       8. DUT ZED sends ZCL On/Off On command
       9. DUT ZED sends ZCL On/Off Off command
       10. DUT ZED sends ZCL On/Off On command
       11. DUT ZED sends ZCL On/Off Off command

Expected outcome:

       1. TH ZC creates a network

       2. TH ZR1, TH ZR2 starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

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

       6. DUT ZED sends ZCL On/Off On command to TH ZR1 and TH ZR2

       7. DUT receives APS Ack from TH ZR1 and TH ZR2

       8. DUT ZED sends ZCL On/Off Off command to TH ZR1

       9. DUT ZED does not receive APS ACK from TH ZR1

       10. DUT ZED sends ZCL On/Off Off command to TH ZR2

       11. DUT ZED receives APS ACK from TH ZR2

Test steps 8-9 and 10-11 can appear in reverse order 10-11 before 8-9

       12. DUT ZED sends ZCL On/Off On command to TH ZR1 and TH ZR2

       13. DUT receives APS Ack from TH ZR1 and TH ZR2

       14. DUT ZED sends ZCL On/Off Off command to TH ZR1

       15. DUT ZED receives APS ACK from TH ZR1

       16. DUT ZED sends ZCL On/Off Off command to TH ZR2

       17. DUT ZED does not receive APS ACK from TH ZR2

Test steps 14-15 and 16-17 can appear in reverse order 16-17 before 14-15

Trace validation:

       1. There is trace message in the DUT's trace ">> light_control_send_on_off, param %hd"

       2. There is trace message in the DUT's trace ">> light_control_send_on_off_cb, param = %hd, status = %hd"

       3. Param in step 1 is the same as param in step 2

    4. Repeat steps 1-3 three times (messages from the steps 1 and 2 should appear in the DUT's trace exactly 4 times!)

    5. The is no trace message "TEST FAILED" in the DUT's trace
