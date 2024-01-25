R22 - network opening after reboot
RTP_BDB_16 - the end-device does not send Permit Join Request

Objective:

    To confirm the end-device will not send Permit Join Request and will not receive ZB_BDB_SIGNAL_STEERING signal after reboot

Devices:

    1. TH  - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used
    2. The DUT ZED does not erase NVRAM at start

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for association of DUT ZED and TH ZC
    4. Reboot DUT ZED
    5. Wait for 20 seconds
    6. Reboot DUT ZED
    7. Wait for DUT ZED send Permit Join Request

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC 
       (DUT ZED does not send Permit Join Request after association process).

        The trace should contain the following strings:
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 0

    3.1. DUT ZED starts after first reboot and rejoins to the network
    
    3.2. DUT ZED does not send Permit Join Request

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0

    4.1. DUT ZED starts after second reboot and rejoins to the network
    
    4.2. DUT ZED sends Permit Join Request

        The trace should contain the following string:
            - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0
            - trigger_steering(), status 1
            - signal: ZB_BDB_SIGNAL_STEERING, status 0

    5. DUT ZED does not fail with assertion error during the test procedure
