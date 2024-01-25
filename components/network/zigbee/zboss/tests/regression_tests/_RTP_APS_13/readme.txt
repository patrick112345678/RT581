ZOI-95 - No packet sending between two local EPs in one group
RTP_APS_13 - Demonstrates sending command to a group; the command is sending from the one local EP to another local one.

Objective:

    To confirm that the device will send group commands to the local EP too

Devices:

    1. DUT - ZC
    2. TH - ZR

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR
    3.1. Wait for DUT ZC send Add Group to TH ZR
    3.1. Wait for DUT ZR respond Add Group Response to DUT ZC with Success
    3.3. Wait for DUT ZC send ZCL On/Off Toggle command to TH ZR
    4. Wait for DUT ZC print messages about receiving local reports

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC

    3.1. DUT ZC sends ZCL Add Group command to TH ZR

    3.2. DUT ZC sends ZCL Add Group command to the local endpoint

    3.3  DUT ZC sends ZCL On/Off Toggle command to the Group (with TEST_GROUP_ID), it should be recieved by TH ZR and local DUT ZC EP.

    4. Both DUT ZC' and TH ZR' trace should contain the following messages in specified order:

        - Groups Add Group command is recieved
        - On/off Toggle command is recieved

    5. The DUT ZC should recive On/off Toggle command
