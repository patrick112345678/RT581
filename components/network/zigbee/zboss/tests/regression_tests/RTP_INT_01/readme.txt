Bug 11378 - Possible scheduler/memory problems inside multiprotocol examples
RTP_INT_01

Objective:

    To confirm that the interrupts does not called into the ZB_SCHEDULE_CALLBACK() code.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. Connect pins P1.11 and P1.13 to each other on the DUT device

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR
        3. TH ZC sends Buffer Test Requests to DUT ZR

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR gets on the network established by TH ZC

        3. Verify that dut's trace does not contain message: "test_pin_error_timeout_handler(): TEST FAILED"

        4. Verify that dut's trace does not contain message: "test_callback_function(): TEST FAILED"

        5. (Test procedure 3) DUT ZR responds with Buffer Test Response packets to TH ZC with success status.
           Status: Successful Buffer Test (0x00)
