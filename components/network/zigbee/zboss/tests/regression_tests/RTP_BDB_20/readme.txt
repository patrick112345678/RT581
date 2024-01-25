Bug ZOI-80 - The value returned by zb_bdb_is_factory_new() is not updated

RTP_BDB_20 - zb_bdb_is_factory_new() will return actual Factory New status flag

Bug notes:

    [NP] The zb_bdb_is_factory_new() value is not updated after leaving/joining of the network and
        could return invalid Factory New Status.

    [NP] It was decided to completely remove nfn flag from BDB context and
        use bdb_joined() function instead and return !bdb_joined() from zb_bdb_is_factory_new().
        This fix also makes the Factory New Device status more clear (device without information about active network
        or network to rejoin to)

Objective:

    To confirm that the zb_bdb_is_factory_new() function returns actual Factory New status flag in case of
        different BDB procedures

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.
	2. TH ZC should not erase NVRAM automatically at start
	3. DUT ZED should not erase NVRAM automatically at start

Test procedure:

    1. Power on DUT ZED
    2. Wait for DUT ZED print Factory New flag to the trace
    3. Wait for DUT ZED performs startup process, do not found network and print Factory New flag
    4. Power on TH ZC
    5. Wait for DUT ZED perform steering process successfully and print Factory New flag
    6. Wait for DUT ZED try to perform steering process one more time and print Factory New flag
    7. Wait for DUT ZED perform Reset Via Local Action and print Factory New flag
    8. Wait for DUT ZED perform steering process successfully and print Factory New flag
    9. Wait for DUT ZED perform Leave With Rejoin and print Factory New flag
    10. Reboot DUT ZED
    11. Wait for DUT ZED print Factory New flag, rejoin to network and print Factory New flag again
    12. Power off TH ZC
    13. Reboot DUT ZED
    14. Wait for DUT ZED print Factory New flag and start Rejoin Back-Off procedure
    15. Power on TH ZC
    16. Wait for DUT ZED successfully rejoin to the network and print Factory New flag
    17. Wait for DUT ZED print message about successful finish of the test

Expected outcome:

    1. [trace, packets]. DUT ZED successfully starts. The trace should contain the following strings:
        - signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status 0, factory_new 1
        - signal: ZB_BDB_SIGNAL_DEVICE_FIRST_START, status 255, factory_new 1

    2. [trace, packets]. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC.
        List of trace messages:
            - signal: ZB_BDB_SIGNAL_STEERING, status 0, factory_new 0

    3. [trace, packets]. DUT ZED starts steering procedure again and sends Permit Join packet.
        List of trace messages:
           - signal: ZB_BDB_SIGNAL_STEERING, status 0, factory_new 0

    4. [trace, packets] DUT ZED leaves the network.
        List of trace messages:
           - signal: ZB_ZDO_SIGNAL_LEAVE, status 0, factory_new 1

    5. [trace, packets]. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC.
        List of trace messages:
            - signal: ZB_BDB_SIGNAL_STEERING, status 0, factory_new 0

    6. [trace, packets] DUT ZED send Leave with Rejoin command and rejoins to the network.
        List of trace messages:
           - signal: ZB_ZDO_SIGNAL_LEAVE, status 0, factory_new 0
           - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0, factory_new 0

    7. [trace, packets] DUT ZED is rebooted. DUT ZED rejoins to the network.
        List of trace messages:
           - signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status 0, factory_new 0
           - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0, factory_new 0

    8. [trace, packets] DUT ZED is rebooted, tries to find the network via Beacon Request, but does not found it.
        List of trace messages:
           - signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status 0, factory_new 0
           - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 255, factory_new 0
                (this string should be repeated at least once - one time for each unsuccessful scanning via Beacon Request)

    9. [trace, packets] DUT ZED finds TH ZC after its reboot and rejoins to the network.
        List of trace messages:
           - signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status 0, factory_new 0

    10. [trace]. DUT ZED prints successful message about the test finish.
        List of trace messages:
           - Test successfully finished

    11. DUT ZED should not crash with assertion during the test procedure.
