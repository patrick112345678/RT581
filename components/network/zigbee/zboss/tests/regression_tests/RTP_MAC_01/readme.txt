Bug 14316 - [ZBOSS 61] MAC-level packet drops
RTP_MAC_01 - OOM state

Objective:

        To verify ZB_MAC_RX_QUEUE_CAP and ZB_MAC_QUEUE_SIZE values, auto ack disable and enable mechanism and detecting OOM state by zboss

Devices:

	1. CTH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on CTH ZC
        2. Power on DUT ZR
        3. CTH sends 10 Buffer Test Requests while DUT is stopped
        4. CTH sends Buffer Test Request while DUT operates normally

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

        3. DUT stops zdo main loop for 10 seconds

    4. DUT sends 5 MAC ACKs for 10 frames from CTH

        5. DUT restarts zdo main loop

        6. (Test procedure 3) DUT responds on 5 Buffer Test Requests from CTH with 5 Buffer Test Response packets
           Status: Successful Buffer Test (0x00)

        7. (Test Procedure 4) DUT responds on Buffer Test Request from CTH with Buffer Test Response
           Status: Successful Buffer Test (0x00)
