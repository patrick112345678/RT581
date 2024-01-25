R22 - network opening after reboot
RTP_BDB_15 - the coordinator does not automatically send Permit Join Request

Objective:

    To confirm the coordinator will not automatically send Permit Join Request and will not receive ZB_BDB_SIGNAL_STEERING signal after reboot

Devices:

    1. DUT - ZC
    2. TH - ZR

Initial conditions:

    1. All devices are factory new and powered off until used
    2. The DUT ZC does not erase NVRAM at start

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR
    3. Wait for association of DUT ZC and TH ZR
    4. Reboot DUT ZC
    5. Wait for 20 seconds
    6. Reboot DUT ZC
    7. Wait for DUT ZC send Permit Join Request

Expected outcome:

	1. DUT ZC creates a network and sends Permit Join Request

	2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC 

        The trace should contain the following strings:
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 0

    3. DUT ZC starts after first reboot and does not send Permit Join Request.

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0

    4. DUT ZC starts after second reboot and send Permit Join Request

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 2

    5. DUT ZC does not fail with assertion error during the test procedure
