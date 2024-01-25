Bug 14972 - Query Image Callback lacking "manufacturer_code" parameter
RTP_OTA_SRV_02 - "manufacture_code" of Query Image Callback

Objective:

        Make sure that manufacture_code parameter from Query Next Image Request is can be correctly passed to application

Devices:

	1. DUT - ZC
        2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR

Expected outcome:

    1. TH performs OTA Upgrade procedure

        2. There is trace msg in dut's trace: "test_zcl_device_cb(): manufacturer_code = 0x7b - test OK"

---------------------------------------------------------------------------------------------------
Bug 15034 - OTA PICS not fully fulfilled - default response thing
RTP_OTA_SRV_02

Objective:

        Verify that OTA server sends a Default Response with status of NO_IMAGE_AVAILABLE when it receives an Image Block Request for a file that it does not have.

Devices:

	1. DUT - ZC
        2. CTH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on CTH ZR

Expected outcome:

    1. CTH performs OTA Upgrade procedure

        (Found in tests from nordic team)
        11.13.6.5.2 No Image Available
        If either manufacturer code or image type or file version information in the request command is invalid 
        or the OTA upgrade file for the client for some reason has disappeared which  result in the server no longer 
        able to retrieve the file, it SHALL send default response command with NO_IMAGE_AVAILABLE status to the client. 
        After three attempts, if the client keeps getting the default response with the same status, 
        it SHOULD go back to sending Query Next Image Request periodically or waiting for next Image Notify command.
