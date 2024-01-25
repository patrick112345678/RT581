ZB-102 - Network Address Request to 0xfffffff
RTP_NWK_03 - Extended address request into f&b procedure

Objective:

	To confirm that device can communicate with devices that do not include extended source address into nwk payload.

Devices:

	1. TH  - ZC
	2. TH  - ZR
        3. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on TH ZR
        3. Power on DUT ZED
        4. Wait for TH ZR and DUT ZED joins the network with TH ZC as a parent
        5. Wait for TH ZR performs F&B procedure with DUT ZED being the F&B initiator
        6. Wait for Toggle command from DUT ZED

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZED and TH ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

        3. DUT ZED sends broadcast Identify Query Command

    4. TH ZR responds DUT with Identify Query Response command without source extended address in the NWK payload.
           ZigBee Network Layer Data
                  Frame Control Field
                  ...0 .... .... .... = Extended Source: False

        5. DUT ZED sends extended address request to TH ZR with TH ZR's address of interest
           ZigBee Device Profile
                  Nwk Addr of Interest: [TH ZR short address]
                  Request Type: Single Device Response (0)
                  Index: 0

    6. TH ZR responds with extended address response without source extended address in the NWK payload
           ZigBee Network Layer Data
                  Frame Control Field
                  ...0 .... .... .... = Extended Source: False

           ZigBee Device Profile
                  Status: Success
                  Extended Address: 00:00:00_01:00:00:00:01

        7. DUT ZED send Active endpoint request to TH ZR

    8. TH ZR responds with Active endpoint response without source extended address in the NWK payload
           ZigBee Network Layer Data
                  Frame Control Field
                  ...0 .... .... .... = Extended Source: False

    9. DUT ZED sends Simple descriptor request to the each TH ZR's endpoint from the received Active endpoint response

    10. TH ZR responds with Simple descriptor Response packets without source extended address in the NWK payload
           ZigBee Network Layer Data
                  Frame Control Field
                  ...0 .... .... .... = Extended Source: False

    11. DUT ZED sends Toggle command to TH ZR with extended destination address of TH ZR included into NWK payload of the packet.
