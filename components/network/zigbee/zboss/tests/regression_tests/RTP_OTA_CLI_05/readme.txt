Bug 12782 - Does OTA Client Cluster support handling minimal/maximum hardware revision? 
RTP_OTA_CLI_05 - OTA Client device sends hardware version with Query Next Image Request and receive appropriate respond

Objective:

    To confirm that the OTA Cluster can handle minimal/maximum hardware revision

Devices:

    1. TH - ZC (OTA server)
    2. DUT - ZR (OTA client)

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR associate with TH ZC
    4. Wait for DUT ZR bind with TH ZC

    # client HW version < min hw version
    5.1. Wait for DUT ZR receive Image Notify
    5.2. Wait for DUT ZR send Query Next Image Request and receive Query Next Image Response with "No Image Available" status

    # client HW version = min hw version
    6.1. Wait for DUT ZR receive Image Notify
    6.2. Wait for DUT ZR send Query Next Image Request and receive Query Next Image Response with "Success" status
    6.3. Wait for DUT ZR complete upgrade procedure

    # client HW version > min hw version and < max_hw_version
    7.1. Wait for DUT ZR receive Image Notify
    7.2. Wait for DUT ZR send Query Next Image Request and receive Query Next Image Response with "Success" status
    7.3. Wait for DUT ZR complete upgrade procedure

    # client HW version = max_hw_version
    8.1. Wait for DUT ZR receive Image Notify
    8.2. Wait for DUT ZR send Query Next Image Request and receive Query Next Image Response with "Success" status
    8.3. Wait for DUT ZR complete upgrade procedure

    # client HW version > max_hw_version
    9.1. Wait for DUT ZR receive Image Notify
    9.2. Wait for DUT ZR send Query Next Image Request and receive Query Next Image Response with "No Image Available" status

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZR performs binding with TH ZC via Match Descriptor Request/Response

    # For test step 5
    4.1. DUT ZR receives Image Notify:
         File Version: 0x01020101
    4.2. DUT ZR send Query Next Image Request
    4.3. DUT ZR receive Query Next Image Response
        Status: Ota No Image Available (0x98)

    # For test step 6
    5.1. DUT ZR receives Image Notify:
         File Version: 0x01020102
    5.2. DUT ZR send Query Next Image Request
    5.3. DUT ZR receive Query Next Image Response
        Status: Success (0x00)
    5.4. DUT ZR completes upgrade procedure. DUT ZR sends Upgrade End Request with "Success" status (0x00) 
         and receives Upgrade End Response

    # For test step 7
    6.1. DUT ZR receives Image Notify:
         File Version: 0x01020103
    6.2. DUT ZR send Query Next Image Request
    6.3. DUT ZR receive Query Next Image Response
        Status: Success (0x00)
    6.4. DUT ZR completes upgrade procedure. DUT ZR sends Upgrade End Request with "Success" status (0x00) 
         and receives Upgrade End Response

    # For test step 8
    7.1. DUT ZR receives Image Notify:
         File Version: 0x01020104
    7.2. DUT ZR send Query Next Image Request
    7.3. DUT ZR receive Query Next Image Response
        Status: Success (0x00)
    7.4. DUT ZR completes upgrade procedure. DUT ZR sends Upgrade End Request with "Success" status (0x00) 
         and receives Upgrade End Response

    # For test step 9
    8.1. DUT ZR receives Image Notify:
         File Version: 0x01020105
    8.2. DUT ZR send Query Next Image Request
    8.3. DUT ZR receive Query Next Image Response
        Status: Ota No Image Available (0x98)