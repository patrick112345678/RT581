Bug 13663 - OTA upgrade cluster parameters 
RTP_OTA_CLI_06 - OTA Upgrade client without Time Cluster uses the difference in seconds between Upgrade Time and Current Time as delay

Objective:

    To confirm that the OTA Upgrade client without Time Cluster will calculate the difference in seconds between Upgrade Time and Current Time 
    and use it as upgrade delay

Devices:

    1. TH - ZC (OTA server)
    2. DUT - ZR (OTA client)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR associate with TH ZC
    4. Wait for DUT ZR bind with TH ZC
    5.1. Wait for DUT ZR start OTA upgrade procedure
    5.2. Wait for DUT ZR complete upgrade image receiving and receive Upgrade End Response
    5.3. Wait for DUT ZR apply received image after calculated delay

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZR applies received image after calculated delay. The trace should contain the following strings:
        - Device is still not upgraded
        - device cb with status ZB_ZCL_OTA_UPGRADE_STATUS_FINISH is called
        - Device is upgraded
