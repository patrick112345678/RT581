Bug 13146 - Bug in Finding & Binding initiator Simple Descriptor Response receiving side 
RTP_BDB_09 - Finding and binding initiator can handle several simultaneous identify responses 

Objective:

	To confirm that the finding and binding initiator can handle several simultaneous identify responses.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

	2. TH ZR has two endpoints

Test procedure:

	1. Power on DUT ZC
	2. Power on TH ZR
    3. Wait for DUT ZC send Identify Query Request and receive two identify query responses for each of TH ZR endpoints
    4. Wait for DUT ZC receive Binding Table Request and send Binding Table Responses

Expected outcome:

	1. DUT ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3. DUT ZC sends Identify Query Request and successfully receives two identify query responses for each of TH ZR endpoints (10 and 11).
	Then DUT ZC should add 4 entries in binding table (for Basic and Identify clusters for each endpoint). 

	4. DUT ZC receives Binding Table Request end sends Binding Table Responses with Table Size = 4.
    
	Binding Table
        Bind
            Source Endpoint: 143
            Cluster: 0x0003 (Identify)
            Destination Endpoint: 10
        Bind
            Source Endpoint: 143
            Cluster: 0x0000 (Basic)
            Destination Endpoint: 10
        Bind
            Source Endpoint: 143
            Cluster: 0x0003 (Identify)
            Destination Endpoint: 11
        Bind
            Source Endpoint: 143
            Cluster: 0x0000 (Basic)
            Destination Endpoint: 11