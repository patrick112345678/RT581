R22 - ZB_ZDO_SIGNAL_LEAVE does not come after local zdo_mgmt_leave_req with rejoin
RTP_ZDO_04 - The application receive ZB_ZDO_SIGNAL_LEAVE after local leave

Objective:

    To confirm that the application will receive ZB_ZDO_SIGNAL_LEAVE after local leave.

Devices:

    1. TH  - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED leaves the network
    4. Wait for DUT ZED rejoin to the network

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED associates TH ZC

    3. DUT ZED leaves the network and sends Leave command. The trace should contain the following string:
        - signal: ZB_ZDO_SIGNAL_LEAVE, status 0

    4. DUT ZED rejoins to the network