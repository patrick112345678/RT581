ZOI-637 - WWAH Door Lock. Test 11. ZCL command response not received
RTP_ZCL_23 - WWAH Test 11 (Pending Network Update)

Objective:

    To confirm that the device will pass WWAH Test 11 (Pending Network Update)

Devices:

    1. TH - ZC
    2. DUT - ZR or ZED (i.g. WWAH Door Lock)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure and expected outcome (see WWAH Zigbee Cluster Test Plan):

    Base Case

    a) TH to send a Read Attribute command, specifying the
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes to
    DUT, verify that the DUT sends back a Read Attribute response with
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes with
    value of 0xFF/0xFFFF.

    No Changes

    b) TH to send a Set Pending Network Update command, specifying arguments of
    current Channel and PANID to DUT.

    c) TH to send a Read Attribute command, specifying the
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes to
    DUT, verify that the DUT sends back a Read Attribute response with
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes with
    value of current channel/PANID.

    d) TH to send NWK Mgmt Network Update command to DUT, specifying a new
    Channel. Verify DUT does not attempt change (no rejoin).

    e) TH to send NWK Network Update command to DUT, specifying a new PANID.
    Verify DUT does not attempt change (no rejoin).

    Change Channel

    f) TH to send a Set Pending Network Update command, specifying arguments of
    new Channel and current PANID to DUT.

    g) TH to send a Read Attribute command, specifying the
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes to
    DUT, verify that the DUT sends back a Read Attribute response with
    PendingNetworkUpdateChannel, PendingNetworkUpdatePANID attributes with
    value of new channel/current PANID.

    h) TH to send NWK Mgmt Network Update command to DUT, specifying a different
    new Channel. Verify DUT does not attempt change (no rejoin).

    i) TH to send NWK Network Update command to DUT, specifying a new PANID.
    Verify DUT does not attempt change PANID (eg. no rejoin, etc.).

    j) TH to send NWK Mgmt Network Update command to DUT, specifying the new
    Channel. Change TH to new channel, Verify DUT changes to the new channel,
    either automatically or via rejoin.

    Change PANID

    k) TH to send a Set Pending Network Update command, specifying arguments of
    current Channel and new PANID to DUT.

    NOTE: RTP TH will repeat test step g) here to verify that DUT responds with current channel and new PANID in
        Read Attributes response

    l) TH to send NWK Mgmt Network Update command to DUT, specifying a new
    Channel. Verify DUT does not attempt change (no rejoin).

    m) TH to send NWK Network Update command to DUT, specifying a different new
    PANID. Verify DUT does not attempt change (no rejoin).

    n) TH to send NWK Network Update command to DUT, specifying the new PANID.
    Change TH to new PANID, Verify DUT changes to the new PANID, either
    automatically or via rejoin.

    NOTE: RTP TH will repeat test step g) here to verify that DUT really had changed its PANID
