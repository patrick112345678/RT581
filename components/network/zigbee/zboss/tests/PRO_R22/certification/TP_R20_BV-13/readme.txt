10.67 TP/R20/BV-13  NWK Leave Request Processing – ZR (CCB1279)
Objective: To confirm that a router can be configured to ignore a leave request. This test
is only necessary if the device supports the optional NIB value nwkLeaveRequestAllowed in its PICs document.
DUT: ZR

gZC
DUT ZR

EPID = 0x0000 0000 0000 0001
Radio range PAN ID = 0x1aaa
Short Address = 0x0000
Long Address = 0x AAAA AAAA AAAA AAAA

EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address = 0x 0000 0001 0000 0000

Initial conditions:
1. Reset all nodes;
2. Set gZC under target stack profile, gZC as coordinator starts a PAN = 0x1AAA network;
3. DUT ZR is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE;
3. NWK Security is disabled on all devices;

Test procedure:
1. Join device DUT ZR to gZC.
2. Configure DUT ZR to disable NWK leave processing (nwkLeaveRequestAllowed = FALSE)
3. Have gZC issue a NWK leave request for DUT ZR.
4. Wait 20 seconds for a link status from the DUT ZR.
5. Configure DUT ZR to allow NWK leave processing (nwkLeaveRequestAllowed=TRUE)
6. Have gZC issue a NWK leave request for DUT ZR.
7. Wait 20 seconds for a link status from the DUT ZR.

Pass Verdict:
1. DUT ZR shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2. DUT ZR is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3. DUT ZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
4. gZC issues a NWK Command unicast to DUT ZR with command ID of 0x04 (Leave), with Command options set to 0x40 (Request = TRUE).
5. DUT ZR does not leave the network and does not issue a NWK leave broadcast.
6. DUT ZR sends a NWK command broadcast, link status, indicating it is still operating on the network.
7. gZC issues a NWK Command unicast to DUT ZR with command ID of 0x04 (Leave), with Command options set to 0x40 (Request = TRUE).
8. DUT ZR issues a broadcast NWK leave command with options set to x00. DUT ZR leaves the network.
9. DUT ZR does not send a link status broadcast.

Fail Verdict:
1. DUT ZR does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2. DUT ZR is not able to complete the MAC association sequence.
3. DUT ZR does not issue a device announcement.
4. gZC does not issue a NWK leave request or Command options is not set 0x40.
5. DUT ZR leaves the network. DUT ZR issues a NWK leave broadcast.
6. DUT ZR does not send a link status broadcast.
7. gZC does not issue a NWK leave request or Command options is not set 0x40.
8. DUT ZR does not issue a broadcast NWK leave command. DUT ZR does not leave the network.
9. DUT ZR sends a NWK command broadcast, link status, indicating it is still operating on the network.




