R22 - Transmission via binding. aps_ack_frame_handle() 
RTP_APS_08 - receiving of two APS ACKs from the same recipient during binding transmission does not lead to address lock leaks

Objective:

       To confirm that the receiving of two APS ACKs from the same recipient during binding transmission does not lead to address lock leaks

Devices:

       1. TH - ZC
       2. DUT - ZED (light switch)
       3. TH - ZR1 (bulb)
       4. TH - ZR2 (bulb)

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. [APP], [APS] trace are enabled on the DUT

Test procedure:

       1. Power on TH ZC
       2. Power on TH ZR1, TH ZR2, TH ZR3, TH ZR4
       3. Power on DUT ZED
       4. Wait for TH ZR1, TH ZR2, TH ZR3, TH ZR4 associate with TH ZC
       5. Wait for DUT ZED associate with TH ZC
       6.1. Wait for DUT ZED send Match Descriptor Request
       6.2. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
       7. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
       8.1. Wait for DUT ZED send ZCL On/Off Toggle command
       8.2. Wait for DUT ZED receive the first APS ACK from TH ZR2
       8.3. Wait for DUT ZED receive the second APS ACK from TH ZR2

Expected outcome:

       1. TH ZC creates a network

       2. TH ZR1, TH ZR2, TH ZR3, TH ZR4 starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       4. DUT ZED performs binding with TH ZR1, TH ZR2, TH ZR3, TH ZR4

       5. DUT ZED send ZCL On/Off Toggle command

       6.1. DUT ZED receives the first APS ACK from TH ZR2.
              The trace should contain the following strings:
                     - +aps_ack_frame_handle, aps_counter=%d (%d - some integer, this APS counter should be equal to APS counter from ACK packet)
                     - finally done with this data pkt i 0
                     - -aps_ack_frame_handle (the second string should be located between the first and the third strings)

                     - TH ZR2 addr lock count before second ack %d (%d - some integer)

       6.2. DUT ZED receives the second APS ACK from TH ZR2.
              The trace should contain the following strings:
                     - +aps_ack_frame_handle, aps_counter=%d (%d - some integer, this APS counter should be equal to APS counter from the step 6.1.)
                     - No entry to this ACK - just drop it
                     - -aps_ack_frame_handle (the second string should be located between the first and the third strings)

                     - TH ZR2 addr lock count after second ack %d (%d - some integer, this number should be equal to the number from the step 6.1.)
              