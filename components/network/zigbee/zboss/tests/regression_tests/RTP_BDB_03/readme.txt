Bug 14552 - Recommendation for BDB factory reset
RTP_BDB_03 - reset to factory defaults during association

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

        2. DUT ZR sends NWK Leave Command and leaves the network
               Command Identifier: Leave (0x04)
                       ..0. .... = Rejoin: False
                       .0.. .... = Request: False
                       0... .... = Remove Children: False

        3. Repeat steps 1 and 2 seven times

    4. There is 8 trace messages in the dut's trace: "zb_bdb_reset_via_local_action param %hd: send leave to myself"

    5. There is 8 trace messages in the dut's trace: "signal: ZB_ZDO_SIGNAL_LEAVE, status 0"

        6. DUT ZR associates to TH ZC

        7. There is a trace message in the dut's trace: "test_trigger_steering(): test finished"

=======================================================================================================
RTP_BDB_NOP_03 - reset to factory defaults during association (no parent)

Objective:

	To confirm that BDB factory reset works correctly right after the scan process.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

        1. (Test steps 1 - 4) Power on DUT ZR
        2. (Test steps 1 - 4) Wait for 40 beacon requests from the DUT ZR
	3. (Test step 5) Power on TH ZC

Expected outcome:

	1. DUT ZR sends 5 Beacon requests

        2. Repeat steps 1 seven times

    3. There is 8 trace messages in the dut's trace: "zb_bdb_reset_via_local_action param %hd: send leave to myself"

    4. There is 8 trace messages in the dut's trace: "signal: ZB_ZDO_SIGNAL_LEAVE, status 0"

        5. DUT ZR associates to TH ZC

        6. There is a trace message in the dut's trace: "test_trigger_steering(): test finished"
