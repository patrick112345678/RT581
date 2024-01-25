Bug ZOI-137 - Device gets kicked out of the network if it does not finish the first association
RTP_BDB_21 - Parent device will not remove child device from the network in case of successful commissioning
    after previously failed authorization attempt

Objective:

    To confirm that the parent device will not remove child device from the network child device in case of successful commissioning
        after previously failed authorization attempt.

Devices:

    1. TH  - ZED
    2. DUT - ZC

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZED
    3. Wait for TH ZED perform first (unsuccessful) commissioning attempt with DUT ZC, and DUT ZC send Transport Key with network key.
        TH ZED should skip Transport Key packet and should not to continue commissioning.
    4. Wait for TH ZED perform second (successful) commissioning attempt and get on DUT
    5. Wait for 20 seconds (ZB_DEFAULT_BDB_TRUST_CENTER_NODE_JOIN_TIMEOUT is 15 seconds) and check that DUT ZC
        will not remove TH ZED device from the network

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZED starts commissioning and tries to get on the network established by DUT ZC.
        DUT ZC sends Transport Key with Network Key, but TH ZED skips the packet and stops commissioning.
        This commissioning attempt should fail.

    3. TH ZED starts commissioning again and gets on the network established by DUT ZC.
        This commissioning attempt should succeed.

    4. DUT ZC doesn't send Leave command to TH ZED and doesn't remove that device
        from the network during the test procedure (at least 20 seconds after the second Transport Key packet).
