Bug 14315 - [ZBOSS 61] Interrupt handling in ZBOSS 
RTP_INT_02 - There should not be difference between ZB_ENABLE_ALL_INTER() and ZB_DISABLE_ALL_INTER() calls count

Objective:

	To confirm that the ZB_ENABLE_ALL_INTER() and ZB_DISABLE_ALL_INTER() called in the right order

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZR
	3. Wait for DUT ZR start

Expected outcome:

	1. ZC and ZR starts successfully

	2. The DUT ZR trace contains string "OSIF interrupts enable/disable balance 1". 
	The number may have other value, but it should not be greater than 1 or lesser than -1.