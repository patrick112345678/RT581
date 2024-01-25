ZOI-623 - WWAH Door Lock. Test 19: OTA attempt breaks the test?
RTP_ZCL_22 - WWAH Test 19 (TC Security On Ntwk Key Rotation)

Objective:

    To confirm that the device will pass WWAH Test 19 (TC Security On Ntwk Key Rotation)

Devices:

    1. TH - ZC
    2. DUT - ZR or ZED (i.g. WWAH Door Lock)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure and expected outcome (see WWAH Zigbee Cluster Test Plan):

    The initial network key used for this test will be referred to as Key #1.

    a) TH to send a Read Attribute command, specifying the
    TCSecurityOnNtwkKeyRotationEnabled attribute to DUT, verify that the DUT
    sends back a Read Attribute response with
    TCSecurityOnNtwkKeyRotationEnabled attribute with value=FALSE. These
    messages must be encrypted at NWK by Key #1 and APS-encrypted.

    b) TH to broadcast APS Transport Key command (0x05) containing new Key #2
    encrypted using Key #1 followed by APS Switch Key command (0x09) encrypted
    using Key #2.

    c) Verify DUT begins to use the new network key. A rejoin might result for some
    devices.

    d) TH to send Enable TC Security On Network Key Rotation command to DUT,
    encrypted at NWK by Key #2 and APS-encrypted.

    e) TH to send a Read Attribute command, specifying the
    TCSecurityOnNtwkKeyRotationEnabled attribute to DUT, verify that the DUT
    sends back a Read Attribute response with
    TCSecurityOnNtwkKeyRotationEnabled attribute with value=TRUE. These
    messages must be encrypted at NWK by Key #2 and APS-encrypted.

    f) TH to broadcast APS Transport Key command (0x05) containing new Key #3
    encrypted using Key #2 followed by APS Switch Key command (0x09) encrypted
    using Key #3.

    g) Verify DUT does NOT begin to use the new network key.

    h) TH to send APS Transport Key command (0x05) containing new Key #4
    encrypted using Key #2 via unicast without APS key Encryption to DUT followed
    by APS Switch Key command (0x09) encrypted using Key #4 via broadcast.

    i) Verify DUT does NOT begin to use the new network key.

    j) TH to send APS Transport Key command (0x05) containing new Key #5
    encrypted using Key #2 via unicast with APS key Encryption to DUT followed by
    APS Switch Key command (0x09) encrypted using Key #5 via broadcast. A rejoin
    might result for some devices.

    k) Verify DUT begins to use the new network Key #5.
