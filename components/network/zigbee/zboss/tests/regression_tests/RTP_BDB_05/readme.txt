Bug 14404 - Coordinator address turning to 0xFFFF
RTP_BDB_05 - correct ic usage

Objective:

        Confirm that TC correctly joins other devices using ic keys after reset with erasing the persistent storage.

Devices:

	1. DUT - ZC
	2. TH  - ZR1
	3. TH1  - ZR2
	4. TH2  - ZR2

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR1
        3. Power on TH1 ZR2
        4. Power off all devices
	5. Power on DUT ZC
        6. Power on TH ZR1
        7. Power on TH2 ZR2

Expected outcome:

	1. DUT ZC creates a network

        2. (Test procedure 1 - 3) TH ZR1 and TH1 ZR2 successfully joined and authorized with DUT ZC

        3. (Test procedure 5 - 6) TH ZR1 successfully joined and authorized with DUT ZC

        4. (Test procedure 7) TH ZR2 shall have successful association without authorization(No device announcement).

        5. DUT ZC sends Remove Device to TH2 ZR2

        6. After Remove Device command shall be send Link Quality Request. Source short address shall not be 0xffff.
