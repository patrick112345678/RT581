Bug 12730 - RX statistics
RTP_OSIF_01 - The RX and TX statistics is collected and available for usage

Objective:

	To confirm that there is ability to override zb_trace_msg_port_do() function and process messages data

MAIN TEST PROCEDURE:

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZR
	3. Wait for DUT ZR print radio statistics to trace
	4. Wait for DUT ZR send Buffer Test Request and receive Buffer Test Response
	5. Wait for DUT ZR print radio statistics to trace
	6. Power off TH ZC
	7. Wait for DUT ZR send Buffer Test Request
	8. Wait for DUT ZR print radio statistics to trace

Expected outcome:

	1. TH ZC starts successfully

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. DUT ZR prints the radio statistics to the trace. The statistics has the following format:

		NRF52840 radio stats:
		rx_successful=X
		rx_err_none=X
		rx_err_invalid_frame=X
		rx_err_invalid_fcs=X
		rx_err_invalid_dest_addr=X
		rx_err_runtime=X
		rx_err_timeslot_ended=X
		rx_err_aborted=X
		tx_successful=X
		tx_err_none=X
		tx_err_busy_channel=X
		tx_err_invalid_ack=X
		tx_err_no_mem=X
		tx_err_timeslot_ended=X
		tx_err_no_ack=X
		tx_err_aborted=X
		tx_err_timeslot_denied=X

	Where X is a decimal number.

	4. DUT ZR sends Buffer Test Request to TH ZC and receives Buffer Test Response

	5. DUT ZR prints the radio statistics to the trace. The following conditions must be true:

		- The difference between rx_successful and rx_successful from step 3 >= 1
		- The difference between tx_successful and tx_successful from step 3 >= 2

	6. After the TH ZC is powered off, DUT ZR sends Buffer Test Request to TH ZC and does not receive Buffer Test Response

	7. DUT ZR prints the radio statistics to the trace. The following conditions must be true:

		- The difference between tx_err_no_ack and tx_err_no_ack from step 5 >= 4

		Additionally, it is possible to check the following conditions:
			- The rx_err_invalid_dest_addr >= the count of MAC acks (from TH ZC to DUT ZR)
			- The rx_err_invalid_frame >= the count of Link status packets (from TH ZC to DUT ZR)

ADDITIONAL TEST PROCEDURE:

Devices:

	1. CTH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on CTH ZC
	2. Power on DUT ZR
	3. Wait for DUT ZR print radio statistics to trace
	4. Wait for DUT ZR send Buffer Test Request and receive Buffer Test Response
	5. Wait for DUT ZR print radio statistics to trace
	6. Power off CTH ZC
	7. Wait for DUT ZR send Buffer Test Request
	8. Wait for DUT ZR print radio statistics to trace
	9. Power on CTH ZC
	10. Wait for CTH ZC send to DUT ZR some Invalid Frame packet (packet with reserved frame type sub field)
	11. Wait for CTH ZC send to DUT ZR some packet with Invalid FCS
	12. Wait for CTH ZC send some packet with any destination address except address of DUT ZR
	13. Wait for DUT ZR print radio statistics to trace

Expected outcome:

	1. CTH ZC starts successfully

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by CTH ZC

	3. DUT ZR prints the radio statistics to the trace. The statistics has the following format:

		NRF52840 radio stats:
		rx_successful=X
		rx_err_none=X
		rx_err_invalid_frame=X
		rx_err_invalid_fcs=X
		rx_err_invalid_dest_addr=X
		rx_err_runtime=X
		rx_err_timeslot_ended=X
		rx_err_aborted=X
		tx_successful=X
		tx_err_none=X
		tx_err_busy_channel=X
		tx_err_invalid_ack=X
		tx_err_no_mem=X
		tx_err_timeslot_ended=X
		tx_err_no_ack=X
		tx_err_aborted=X
		tx_err_timeslot_denied=X

	Where X is a decimal number.

	4. DUT ZR sends Buffer Test Request to CTH ZC and receives Buffer Test Response

	5. DUT ZR prints the radio statistics to the trace. The following conditions must be true:

		- The difference between rx_successful and rx_successful from step 3 >= 2
		- The difference between tx_successful and tx_successful from step 3 >= 3

	6. After the CTH ZC is powered off, DUT ZR sends Buffer Test Request to CTH ZC and does not receive Buffer Test Response

	7. DUT ZR prints the radio statistics to the trace. The following conditions must be true:

		- The difference between tx_err_no_ack and tx_err_no_ack from step 5 >= 4

	8. CTH ZC sends to DUT ZR some Invalid Frame packet (packet with reserved frame type sub field)

	9. CTH ZC sends to DUT ZR some packet with Invalid FCS

	10. CTH ZC sends some packet with any destination address except address of DUT ZR

	11. DUT ZR prints the radio statistics to the trace. The following conditions must be true:

		- The difference between rx_err_invalid_frame and rx_err_invalid_frame from step 7 >= 1
		- The difference between rx_err_invalid_fcs and rx_err_invalid_fcs from step 7 >= 1
		- The difference between rx_err_invalid_dest_addr and rx_err_invalid_dest_addr from step 7 >= 1

		Additionally, it is possible to check the following conditions:
			- The rx_err_invalid_dest_addr >= the count of MAC acks (from CTH ZC to DUT ZR)
			- The rx_err_invalid_frame >= the count of Link status packets (from CTH ZC to DUT ZR)
