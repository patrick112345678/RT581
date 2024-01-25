Bug ZOI-Internal-82817 (Celoxis) - Infinite loop after reporting configuration before binding

RTP_ZCL_18 - Putting reporting configuration with specified max interval leads to infinite loop in reporting

Objective:

    To confirm that it is possible to configure reporting before binding server cluster to client

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED send ZCL Report Attributes command for Current Level attribute with value equals to 50
    4. Wait for DUT ZED send ZCL Report Attributes command for Current Level attribute with value equals to 100

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED sends ZCL Report Attributes command with the "Current Level" attribute
        Attribute Field
            Attribute: Current Level (0x0000)
            Data Type: 8-Bit Unsigned Integer (0x20)
            Current Level: 50

    4. DUT ZED sends ZCL Report Attributes command with the "Current Level" attribute
        Attribute Field
            Attribute: Current Level (0x0000)
            Data Type: 8-Bit Unsigned Integer (0x20)
            Current Level: 100

    5. The DUT ZED device do not crash, have assertion error or get stuck