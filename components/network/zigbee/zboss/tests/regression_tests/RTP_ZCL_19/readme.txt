Bug ZOI-114 - Binary input cluster : Error in writing attribute 'active text'

RTP_ZCL_19 - Writing "Active Text" attribute to Binary Input cluster does not lead to device crash

Objective:

    To confirm that the device will not crash after writing attribute value with space

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED receive ZCL Read Attributes command for "Active Text" attribute and respond with Read Attributes Response
    4. Wait for DUT ZED receive ZCL Write Attributes command for "Active Text" attribute and respond with Write Attributes Response
    5. Wait for DUT ZED receive ZCL Read Attributes command for "Active Text" attribute and respond with Read Attributes Response

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED receives ZCL Read Attributes command for "Active Text" attribute

    4. DUT ZED responds with Read Attributes Response with "Unsupported Attribute" status

    5. DUT ZED receives ZCL Write Attributes command for the "Active Text" attribute
        Attribute Field, String: two words
            Attribute: Active Text (0x0004)
            Data Type: Character String (0x42)
            String: two words

    6. DUT ZED responds with Write Attributes Response with "Unsupported Attribute" status

    7. DUT ZED receives ZCL Read Attributes command for "Active Text" attribute

    8. DUT ZED responds with Read Attributes Response with "Unsupported Attribute" status

    9. DUT ZED does not crash or fail with assertion during the test procedure