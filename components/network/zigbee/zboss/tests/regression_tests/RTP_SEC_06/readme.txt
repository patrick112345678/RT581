R22 - Security failure (DUT uses only ic policy)
RTP_SEC_06 - device that uses install codes does not stay on the network after unsuccessful TK decryption 

Objective:

    To confirm that end device that uses install codes will not stay on the network after unsuccessful TK decryption

Devices:

    1. TH  - ZC1
    2. TH  - ZC2
    2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
    2. DUT ZED is built with using IC policy
    3. TH ZC1 is built without using IC policy (to disallow DUT ZED authorization)
    4. TH ZC2 is built with using IC policy (to allow DUT ZED authorization)

Test procedure:

	1. Power on TH ZC1
    2. Power on DUT ZED
    3. Wait for DUT ZED try join to the network created by TH ZC1 and perform keys exchange
    4. Power off TH ZC1, power on TH ZC2
    5. Restart DUT ZED
    6. Wait for DUT ZED get on the network created by TH ZC2

Expected outcome:

    1. TH ZC1 creates a network

    2. DUT ZED try to join to the network created by TH ZC1. 
       DUT ZED receive Transport Key command and does not respond with Device Announcement command (authorization is failed)

    3. TH ZC2 creates a network

    4. DUT ZED gets on the network established by TH ZC2 (successfully joins and performs key exchange)