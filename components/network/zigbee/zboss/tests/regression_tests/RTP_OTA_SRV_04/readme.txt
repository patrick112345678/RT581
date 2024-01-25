R22 - Aborted OTA transmission
RTP_OTA_SRV_04 - device clears binding transmission tables after leave

Objective:

    To confirm that the remove file during OTA upgrade does not cause to stack crash

Devices:

    1. DUT - ZC (OTA server)
    2. TH - ZR (OTA client)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR
    3. Wait for TH ZR associate with DUT ZC
    4. Wait for TH ZR bind with DUT ZC
    5.1. Wait for DUT ZC start OTA upgrade procedure
    5.2. Wait for DUT ZC receive two Image Block Requests and remove active file after them
    5.3. Wait for DUT ZC receive Image Block Request and respond with Default Response (step repeats 3 times)
    5.4. Wait for DUT ZC receive Upgrade End Request and respond with Default Response

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC

    3. TH ZR performs binding with DUT ZC via Match Descriptor Request/Response

    4.1. DUT ZC sends Image Notify ZCL command 

    4.2. DUT ZC receives Query Next Image Request and responds with Query Next Image Response with Success (0x00) status

    4.3. DUT ZC receives Image Block Request and responds with Image Block Response with Success (0x00) status 2 times

    4.4. DUT ZC receives Image Block Request and responds with Default Response 3 times:
        Status: Ota No Image Available (0x98)

    4.5. DUT ZC receives Upgrade End Request with status Status: Ota Abort (0x95) 
    
    4.6. DUT ZC responds with Default Response:
        Status: Success (0x00)