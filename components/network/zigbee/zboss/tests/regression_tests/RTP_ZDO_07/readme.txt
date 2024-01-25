Bug ZOI-297 ZBOSS assert after receiving the leave command
RTP_ZDO_07 - Leave after reboot
Objective:

    Confirm that deveice's address is locked only once after association or reboot and unlocked once after leave.

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Reboot DUT ZED
    4. Wait for DUT ZED receive Leave command (without rejoin) and successfully leave the network

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED successfully rejoines to the network

    4. DUT ZED receives Leave command without Rejoin and successfully leaves the network

    5. Wait for 5 seconds to confirm that the DUT ZED continues to work

    5. DUT ZED should not crash with assertion error during the test procedure
        (the trace should not contain the string "Assertion failed")
