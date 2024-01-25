Bug 15079 - Dereference of NULL pointer in read attribute handler
RTP_ZCL_01 - GLOBAL_CLUSTER_REVISION and unsupported attribute for cluster without attributes

Objective:

    To confirm that the read attribute handler correctly handles GLOBAL_CLUSTER_REVISION and 
        unsupported attributes for clusters without attributes.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR
        3. Wait for DUT ZC send bind request
        4. Wait for 2 read attribute frames from TH ZR

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

    3. DUT ZC successfully binds to TH ZR identify cluster

	4. DUT ZC responds for the read attribute from TH ZR with Read Attribute Response
               Attribute: Unknown (0xfffd)
               Status: Success (0x00)
               Data Type: 16-Bit Unsigned Integer (0x21)
               Uint16: 1 (0x0001)


	5. DUT ZC responds for the read attribute from TH ZR with Read Attribute Response
               Attribute: Unknown (0x0001)
               Status: Unsupported Attribute (0x86)
