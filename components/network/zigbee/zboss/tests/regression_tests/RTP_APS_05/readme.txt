R22 - Light switch and several bulb devices 
RTP_APS_05 - light switch joins to network with several bulb devices and send commands with short interval

Objective:

       To confirm that the light switch can join to network with several bulb devices and can send commands and handle responses with short time interval

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
       8. Wait for DUT ZED repeat the following procedure 5 times:
              8.1. DUT ZED sends ZCL On/Off On command
              8.2. DUT ZED sends Level Control Step Up command
              8.3. DUT ZED sends ZCL On/Off Off command
              8.4. DUT ZED sends Level Control Step Down command
       9. Wait for DUT ZED repeat the following procedure 5 times:
              9.1. DUT ZED sends ZCL On/Off On command
              9.2. DUT ZED sends Level Control Step Up command
              9.3. DUT ZED sends ZCL On/Off Off command
              9.4. DUT ZED sends Level Control Step Down command

Expected outcome:

       1. TH ZC creates a network

       2. TH ZR1, TH ZR2, TH ZR3, TH ZR4 starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       4. DUT ZED repeat the following procedure 5 times (each command is sent with 3 seconds interval):
        4.1. DUT ZED sends ZCL On/Off On command
        4.2. DUT ZED sends Level Control Step Up command
        4.3. DUT ZED sends ZCL On/Off Off command
        4.4. DUT ZED sends Level Control Step Down command

       5. DUT ZED repeat the following procedure 5 times (each command is sent with 1 seconds interval):
        5.1. DUT ZED sends ZCL On/Off On command
        5.2. DUT ZED sends Level Control Step Up command
        5.3. DUT ZED sends ZCL On/Off Off command
        5.4. DUT ZED sends Level Control Step Down command