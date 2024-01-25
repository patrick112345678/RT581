Bug 14536 - Question: zb_secur_apsme_request_key() to request APP_LINK_KEY
BDB_06 - Devices establish end-to-end encryption using Request Key message

Objective:

    To confirm that devices can establish end-to-end encryption using zb_secur_apsme_request_key() for requesting APP_LINK_KEY

Devices:

    1. TH - ZC
    2. DUT  - ZR
    3. TH - ZR

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Power on TH ZR
    4. Wait for DUT ZR send Request Key to TH ZC and receive Transport Key
    5. Wait for DUT ZR send Buffer Test Request and receive Buffer Test Response

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZR and TH ZR successfully joined and authorized with TH ZC

    3.1. DUT ZR sends Request Key (for APP_LINK_KEY) to TH ZC to establish end-to-end encryption with TH ZR
        Command Frame: Request Key
            Command Identifier: Request Key (0x08)
            Key Type: Application Master Key (0x02)
            Partner Address: 00:00:00_01:00:00:00:02 (00:00:00:01:00:00:00:02)
    3.2. DUT ZR receives Transport Key from TH ZC
        Command Frame: Transport Key
            Command Identifier: Transport Key (0x05)
            Key Type: Application Link Key (0x03)
            Key: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx (some key)
            Partner Address: 00:00:00_01:00:00:00:02 (00:00:00:01:00:00:00:02)
            Initiator: True

    4.1. DUT ZR sends Buffer Test Request to TH ZR
        The Request should contain ZigBee Security Header in the APS section
            ZigBee Security Header
                Security Control Field: 0x00, Key Id: Link Key
                [Key: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx] (the key should be equal to key from step 3.2)
	4.2. DUT ZR receives Buffer Test Response from TH ZR
		The Response should contain ZigBee Security Header in the APS section
			ZigBee Security Header
                Security Control Field: 0x00, Key Id: Link Key
                [Key: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx] (the key should be equal to key from step 3.2)