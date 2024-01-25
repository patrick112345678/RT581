Bug ZOI 63 - Address unlock issue for ZDO MGMT Leave req
RTP_ZDO_06 - Leave without rejoin after leave with rejoin does not lead to assertion fail.

Bug notes:
    The reason of the bug is wrong lock/unlock according to the following wrong behavior:
        - ZR locks parent address after association (zb_mlme_associate_confirm)
        - ZR unlocks parent address and set neighbor relationship in neighbor table after leave with rejoin (zb_nwk_do_leave)
        - ZR does not lock parent address again after successful rejoin
        - ZR tries to unlock parent address during leave without rejoin and crashes with assertion error

Objective:

    Confirm that leave without rejoin after leave with rejoin will not lead to assertion fail.

Devices:

    1. TH - ZC
    2. DUT - ZR

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR receive Leave command (with rejoin) and successfully rejoin to the network
    4. Wait for DUT ZR receive Leave command (without rejoin) and successfully leave the network

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZR receives Leave command with Rejoin, successfully leaves and rejoins to the network

    4. DUT ZR receives Leave command without Rejoin and successfully leaves the network

    5. Wait for 5 seconds to confirm that the DUT ZR continues to work

    5. DUT ZR should not crash with assertion error during the test procedure
        (the trace should not contain the string "Assertion failed")
