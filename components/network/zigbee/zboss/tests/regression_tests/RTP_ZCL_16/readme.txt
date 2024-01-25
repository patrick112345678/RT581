Bug 11897 - Request for implementation of Non-ACK'ed APS 
RTP_ZCL_16 - a device can send ZCL packet without APS confirmation (with acknowledge request set to false)

Objective:

    To confirm that a device can send ZCL packet without APS confirmation (with acknowledge request set to false)

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. OnOff attribute initial value of TH ZC is On (0x01)

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED bind with TH ZC (DUT ZED is initiator, TH ZC is target)
    4. Wait for DUT ZED send On Off Toggle command (without APS ACK requesting, without random delay)
    5. Wait for DUT ZED send Off Toggle command (without APS ACK requesting, with random delay)

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED starts finding and binding procedure as initiator and binds with TH ZC

    4. DUT ZED sends On Off Toggle command to TH ZC without ACK requesting:
        ZigBee Application Support Layer Data
            Frame Control Field: Data
                .0.. .... = Acknowledgement Request: False

    5. DUT ZED sends On Off Toggle command to TH ZC without ACK requesting and with random delay.
    The interval between start of this step and the end of the step 4. should not be greater than 9 seconds
    (8 seconds + 1 for ZBOSS internal operations):
        
    ZigBee Application Support Layer Data
            Frame Control Field: Data
                .0.. .... = Acknowledgement Request: False