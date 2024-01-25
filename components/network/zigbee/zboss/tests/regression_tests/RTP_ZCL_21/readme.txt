ZOI-677 - WWAH Door Lock. Test 20. No Check-in Request
RTP_ZCL_21 - WWAH Test 20 (Bad Parent Recovery (End Devices))

Objective:

    To confirm that the device will pass WWAH Test 20 (Bad Parent Recovery (End Devices))

Devices:

    1. TH - ZC
    2. DUT - ZR or ZED (i.g. WWAH Door Lock)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure and expected outcome (see WWAH Zigbee Cluster Test Plan):

    a) TH to send a Read Attribute command, specifying the
    WWAHBadParentRecoveryEnabled attribute to DUT, verify that the DUT sends
    back a Read Attribute response with WWAHBadParentRecoveryEnabled
    attribute with value=FALSE.

    b) TH to send Enable WWAH Bad Parent Recovery command to DUT.

    c) TH to send a Read Attribute command, specifying the
    WWAHBadParentRecoveryEnabled attribute to DUT, verify that the DUT sends
    back a Read Attribute response with WWAHBadParentRecoveryEnabled
    attribute with value=TRUE.

    If Sleepy End Device:
        d) TH to block/ignore all Poll Control Cluster Checkin requests from DUT
        e) Wait for 3 cycles of the Poll Control Checkin

    The following test steps are not implemented yet:

    If Non-sleepy End Device
        f) TH to drop all messages from DUT for 24 hours (no APS ACKs)
        g) TH to verify DUT performs a rejoin
        h) TH to resume proper handling of all Poll Control Cluster Checkin requests from
        DUT
        i) Force bad RSSI on all messages sent from TH to DUT
        j) Wait 24 hours
        k) TH to verify DUT performs a rejoin
        l) TH to send Disable WWAH Bad Parent Recovery command to DUT.
        m) TH to send a Read Attribute command, specifying the
        WWAHBadParentRecoveryEnabled attribute to DUT, verify that the DUT sends
        back a Read Attribute response with WWAHBadParentRecoveryEnabled
        attribute with value=FALSE.