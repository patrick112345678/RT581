Bug 14552 - Recommendation for BDB factory reset
RTP_BDB_13 - reset to factory defaults during rejoin backoff

Objective:

	To confirm that BDB factory reset works correctly right after the association process.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR

Expected outcome:

	1. DUT ZR associates to TH ZC

    	2. DUT ZR rejoins to TH ZC

        3. DUT ZR sends NWK Leave Command and leaves the network
               Command Identifier: Leave (0x04)
                       ..0. .... = Rejoin: False
                       .0.. .... = Request: False
                       0... .... = Remove Children: False

    4. Repeat steps 1, 2 and 3 seven times

    5. There is 8 trace messages in the dut's trace: "zb_bdb_reset_via_local_action param %hd: send leave to myself"

    6. There is 8 trace messages in the dut's trace: "signal: ZB_ZDO_SIGNAL_LEAVE, status 0"

    7. DUT ZR associates to TH ZC

    8. There is a trace message in the dut's trace: "test_trigger_rejoin(): test finished"
