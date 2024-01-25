RTP_MAC_06 - null ieee address association

Objective:

    To confirm that ZC does not accept association for devices with null ieee address

Devices:

    1. DUT - ZC
    2. TH - ZR1
    3. TH - ZR2

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. TH ZR2 has ieee address 00:00:00:00:00:00:00:00

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR1
    3. Wait for TH ZR1 associate with DUT ZC
    4. Power on TH ZR2
    5. Wait for TH ZR2 tries to associate with DUT ZC
    6. Wait for TH ZR1 starts sending beacon requests

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZR1 starts bdb_top_level_commissioning and gets on the network established by DUT ZC

    3. DUT ZC responds with Beacon (Association Permit field is set to True) on Beacon Request from TH ZR2

    4. DUT ZC does not sending Association Response to TH ZR2

    5. TH ZR1 does not rejoins to DUT ZC
