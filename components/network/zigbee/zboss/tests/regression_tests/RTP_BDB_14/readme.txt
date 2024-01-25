R22 - network opening after reboot
RTP_BDB_14 - the router does not automatically send Permit Join Request

Objective:

    To confirm the router will not automatically send Permit Join Request and will not receive ZB_BDB_SIGNAL_STEERING signal after reboot

Devices:

    1. TH  - ZC
    2. DUT - ZR

Initial conditions:

    1. All devices are factory new and powered off until used
    2. The DUT ZR does not erase NVRAM at start

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for association of DUT ZR and TH ZC
    4. Reboot DUT ZR
    5. Wait for 20 seconds
    6. Reboot DUT ZR
    7. Wait for DUT ZR send Permit Join Request

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC 
       (DUT ZR sends Permit Join Request after association process).

        The trace should contain the following strings:
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 0

    3. DUT ZR starts after first reboot and does not send Permit Join Request.

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0

    4. DUT ZR starts after second reboot and send Permit Join Request

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 0

    5. DUT ZR does not fail with assertion error during the test procedure
