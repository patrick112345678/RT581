Bug 14380 - Unimplemented attribute-related ZCL commands 
RTP_ZCL_03 - check behavior of default, undivided and no-response types of ZCL attributes writing

Objective:

	To confirm that the default, undivided and no-response request to write attributes correctly sends and changes attributes values.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR
        3.1. Wait for TH ZR send no-response write attribute request
        3.2. Wait for TH ZR send read attribute request and DUT ZC answer read response
        4.1. Wait for TH ZR send undivided write attribute request
        4.2. Wait for TH ZR send read attribute request and DUT ZC answer read response
        5.1. Wait for TH ZR send default write attribute request with invalid type of first attribute
        5.2. Wait for TH ZR send read attribute request and DUT ZC answer read response
        6.1. Wait for TH ZR send no-response write attribute request with invalid type of first attribute
        6.2. Wait for TH ZR send read attribute request and DUT ZC answer read response
        7.1. Wait for TH ZR send undivided write attribute request with invalid type of first attribute
        7.2. Wait for TH ZR send read attribute request and DUT ZC answer read response


Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3.1. TH ZR sends Write Attributes No Response command for DUT ZC
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 1.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 1.5 seconds

	DUT ZC does not responds with the Write Attributes Response

        3.2. TH ZR sends Read Attributes command for DUT ZC
                Attribute: OnTime (0x4001)
                Attribute: OffWaitTime (0x4002)

        DUT ZC responds with the Read Attributes Response
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 1.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 1.5 seconds


	4.1. TH ZR sends Write Attributes Undivided command for DUT ZC
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.5 seconds

	DUT ZC responds with the Write Attributes Response
               Status: Success (0x00)

        4.2. TH ZR sends Read Attributes command for DUT ZC
                Attribute: OnTime (0x4001)
                Attribute: OffWaitTime (0x4002)

        DUT ZC responds with the Read Attributes Response
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.5 seconds

	5.1. TH ZR sends Write Attributes Default command for DUT ZC with wrong type of first attribute
               Attribute: OnTime (0x4001)
               Data Type: 8-Bit Unsigned Integer (0x20)
               On Time: 3.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 3.5 seconds

	DUT ZC responds with the Write Attributes Response
               Status: Invalid data type (0x8d)
               Attribute: OnTime (0x4001)

        5.2. TH ZR sends Read Attributes command for DUT ZC
                Attribute: OnTime (0x4001)
                Attribute: OffWaitTime (0x4002)

        DUT ZC responds with the Read Attributes Response
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 3.5 seconds

	6.1. TH ZR sends Write Attributes No Response command for DUT ZC with wrong type of first attribute
               Attribute: OnTime (0x4001)
               Data Type: 8-Bit Unsigned Integer (0x20)
               On Time: 4.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 4.5 seconds

	DUT ZC does not respond with the Write Attributes Response

        6.2. TH ZR sends Read Attributes command for DUT ZC
                Attribute: OnTime (0x4001)
                Attribute: OffWaitTime (0x4002)

        DUT ZC responds with the Read Attributes Response
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 4.5 seconds

	7.1. TH ZR sends Write Attributes Undivided command for DUT ZC with wrong type of first attribute
               Attribute: OnTime (0x4001)
               Data Type: 8-Bit Unsigned Integer (0x20)
               On Time: 5.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 5.5 seconds

	DUT ZC responds with the Write Attributes Response
               Status: Invalid data type (0x8d)
               Attribute: OnTime (0x4001)

        7.2. TH ZR sends Read Attributes command for DUT ZC
                Attribute: OnTime (0x4001)
                Attribute: OffWaitTime (0x4002)

        DUT ZC responds with the Read Attributes Response
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 2.0 seconds

               Attribute: OffWaitTime (0x4002)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 4.5 seconds