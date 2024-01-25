TP/PRO/BV-02 End Device – Joins the network
Verify that the end device is capable to join a network and make a device announcement

gZC
DUT ZED1
DUT ZED2

Test procedure:
1. DUT ZED performs startup procedure and joins the PAN network with Rx on idle = false;
2. DUT ZED performs startup procedure and joins the PAN network with Rx on idle = True;

PassFailExpected Outcome
verdict:
1.DUT ZED1 shall issue a MLME_Beacon Request MAC command
frame, and gZC shall respond with beacon.
2. Based on the active scan result DUT ZR is able to complete the MAC
association sequence with gZC and gets new short address from
Coordinator (Generated in a random manner within the range 1 to
0xFFF7)
3. DUT ZED1 issues device Announcement to the network.
4. DUT ZED1 polls gZC and receives message with interval as defined
by stack profile (say 7.5 seconds).
5. DUT ZED2 shall issue a MLME_Beacon Request MAC command
frame, and gZC shall respond with beacon.
6. Based on the active scan result DUT ZED2 is able to complete the
MAC association sequence with gZC and gets new short address from
Coordinator (Generated in a random manner within the range 1 to
0xFFF7)
7. DUT ZED2 issues device Announcement to the gZC.

Fail verdict:
1. DUT ZED1 does not issue a MLME_Beacon Request MAC command
frame, and gZC shall respond with beacon.
2. Based on the active scan result DUT ZED not able to complete the
MAC association sequence with ZC and does not get new short
address (Generated in a random manner within the range 1 to
0xFFF7)
3. DUT ZED1 does not issue device Announcement to the gZC
4. DUT ZED1 does not poll.
5. DUT ZED2 does not issue a MLME_Beacon Request MAC command frame, and gZC shall respond with beacon.
6. Based on the active scan result DUT ZED2 not able to complete the
MAC association sequence with gZC and does not get new short
address (Generated in a random manner within the range 1 to
0xFFF7)
7. DUT ZED2 does not device Announcement to the gZC.
8. ZED1 and ZED2 had the same short address





