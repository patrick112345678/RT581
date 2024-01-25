Bug 11834 - Impossible to read the ZCL Version attribute of Basic Cluster - and others
RTP_HA_03 - ZCL Version attribute of Basic Cluster

Objective:

	To confirm that the read attribute handler correctly handles ZCL Version attribute for basic cluster if DIMMER_SWITCH_CLUSTER_LIST is used.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR
        3. Wait for read attribute ZCL command from TH ZR

Expected outcome:

	1. DUT ZC creates a network

	2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC

        3. DUT ZC performing f&b as a target

    4. DUT ZC responds for the read attribute from TH ZR with Read Attribute Response
               Attribute: ZCL Version (0x0000)
               Status: Success (0x00)
               Data Type: 8-Bit Unsigned Integer (0x20)
               Uint8: 2 (0x02)
