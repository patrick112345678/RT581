R22 - Transmission via binding. zb_aps_clear_after_leave() 
RTP_APS_11 - device clears binding transmission tables after leave

Objective:

    To confirm that the device correctly clears binding transmission tables after leave and do not assert or fail

Devices:

    1. TH - ZC
    2. DUT - ZED (light switch)
    3. TH - ZR1 (bulb)
    4. TH - ZR2 (bulb)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on TH ZR1, TH ZR2
    3. Power on DUT ZED
    4. Wait for TH ZR1, TH ZR2 associate with TH ZC
    5. Wait for DUT ZED associate with TH ZC
    6.1. Wait for DUT ZED send Match Descriptor Request
    6.2. Wait for DUT ZED receive Match Descriptor Response from each of bulbs
    7. Wait for DUT ZED send Extended Address Request to each bulb and receive Extended Address Response
    8.1. Wait for DUT ZED send one ZCL On/Off Toggle command via binding transmission
    8,2. Wait for DUT ZED leave the network and clear binding transmission tables
    8.3. Wait for DUT ZED rejoin to the network
    8.4. Wait for DUT ZED send ZCL On/Off Toggle command via binding transmission

Expected outcome:

    1. TH ZC creates a network

    2. TH ZR1, TH ZR2 starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    4. DUT ZED performs binding with TH ZR1, TH ZR2

    5.1. DUT ZED send 1 ZCL On/Off Toggle command (DUT ZED must not send more than one Toggle commands)

    5.2. DUT ZED send broadcast Leave command.
        The trace should contain the following strings:
            - APS.retrans.hash is empty
            - APS.binding.trans_table is empty

    5.3. DUT ZED rejoins to the network

    5.4. DUT ZED send 2 ZCL On/Off Toggle commands to each TH ZR