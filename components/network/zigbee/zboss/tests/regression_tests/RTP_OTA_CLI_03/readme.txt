Bug 12013 - Implement the suspension in the Image Block Requests in the OTA
RTP_OTA_CLI_03 - halt and resume OTA process

Objective:

        Verify that upgrade status ZB_ZCL_OTA_UPGRADE_STATUS_BUSY halts sending next Image Block Request and zb_zcl_ota_upgrade_resume_client() function successfully resumes OTA process.

Devices:

	1. TH  - ZC
        2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR
        3. TH ZC sends Image Notify Command with OTA Query Jitter value 1

Expected outcome:

	1. DUT starts OTA Upgrade procedure with sending command Query Next Image Request

        2. DUT sends commands Image Block Request with 3 seconds interval

        3. DUT waits 3 seconds before sending Upgrade End Request command
