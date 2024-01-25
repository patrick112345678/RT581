RTP_BDB_NOP_13 - reset to factory defaults during rejoin backoff (no parent)

Objective:

	To confirm that BDB factory reset works correctly right after the scan process.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR

Expected outcome:

        1. DUT ZR associates to TH ZC (Beacon Req., Beacon ...)

        2. DUT ZR sends Beacon Request

        3. Repeat steps 1 and 2 seven times

    4. There is 8 trace messages in the dut's trace: "zb_bdb_reset_via_local_action param %hd: send leave to myself"

    5. There is 8 trace messages in the dut's trace: "signal: ZB_ZDO_SIGNAL_LEAVE, status 0"

        6. DUT ZR associates to TH ZC

        7. There is a trace message in the dut's trace: "test_trigger_rejoin(): test finished"
