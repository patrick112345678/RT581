ZOI-223: MTORR commands and adapters
ZOI-560: Malformed Route Records in case of enabled MTORR and inability to find the last element in Route Discovery Table

RTP_NWK_06 - DUT ZR1 starts Concentrator Mode and sends Many-To-One Route Requests, TH ZED has ability to send Link Quality Request
    through DUT ZR4 without new route discovery (using existing many-to-one route).

Objective:

    To confirm that the DUT ZRs will use Many-to-One Route Request to discover routes:
        - DUT ZR1 should be able to start and stop Concentrator Mode (to start and and to stop send Many-to-One Route Requests).
        - DUT ZR2, DUT ZR3, DUT ZR4 should be able to receive and use MTORR to route packets.

Devices:

    1. TH - ZC
    2. DUT - ZR1
    3. DUT - ZR2
    4. DUT - ZR3
    5. DUT - ZR4
    6. TH - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. Devices visibility should be setup so that each device could see only two neighbor devices according to the network scheme
        ZC -> ZR1 -> ZR2 -> ZR3 -> ZR4 -> ZED (i.g. ZR2 could see only ZR1 and ZR3 devices)

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR1, wait for its commissioning with TH ZC
    3. Power on DUT ZR2, wait for its commissioning with DUT ZR1
    4. Power on DUT ZR3, wait for its commissioning with DUT ZR2
    5. Power on DUT ZR4, wait for its commissioning with DUT ZR3
    6. Power on TH ZED, wait for its commissioning with DUT ZR4
    7. Wait for TH ZED sends Link Quality Request and receive Link Quality Response

Expected outcome:

    1. TH ZC creates a network

    2.1. DUT ZR1 passes commissioning with TH ZC
    2.2. DUT ZR2 passes commissioning with DUT ZR1
    2.3. DUT ZR3 passes commissioning with DUT ZR2
    2.4. DUT ZR4 passes commissioning with DUT ZR3
    2.5. TH ZED passes commissioning with DUT ZR4
    2.6. DUT ZR4 receives Many-to-One Route Request from DUT ZR1 between EO 2.4 and 2.5.

    3.1. TH ZED sends Link Quality Request to DUT ZR4
    3.2. DUT ZR4 doesn't perform new route discovery, but sends Route Record with own short address to DUT ZR3
    3.3. DUT ZR3 adds own short address to Route Record and forwards it to DUT ZR2
    3.4. DUT ZR2 adds own short address to Route Record and forwards it to DUT ZR1
    3.5. DUT ZR4 forwards Link Quality Request to DUT ZR3
    3.6. DUT ZR3 forwards Link Quality Request to DUT ZR2
    3.7. DUT ZR2 forwards Link Quality Request to DUT ZR1
    3.8. DUT ZR2, DUT ZR3, DUT ZR4 should not send any Route Requests to find route to DUT ZR1 after EO 2.4.

    4. TH ZED receives Link Quality Response from DUT ZR1

    5. DUT ZR1 stops sending Many-to-One Route Requests after EO 4.
        NOTE: it is additional condition to check that Concentrator Mode stopping works.
        So, it is needed to validate that DUT ZR1 doesn't send any Many-to-One Route Requests within 30 seconds after EO 4.

Some additional notes:
    - Devices visibility could be setup with using nodes_location.cfg file, so the test is intended for NSNG platform.
    - In some memory configurations routing tables could overflow, but routes entry lifetime is 10 seconds,
        so delay between devices starting could help.
