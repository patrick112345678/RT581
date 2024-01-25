12.8	TP/R21/BV-08 Trust Center Link-Key Update – for TC
Objective: DUT ZC responds to APS Request-Key where key = TC link key; DUT ZC keeps track of whether the key-exchange has succeeded


dutZC - gZR - gZED

dutZC
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = 0x0000
Long Address =
0x AAAA AAAA AAAA AAAA

gZR
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address =
0x 0000 0001 0000 0000

gZED
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address =
0x 0000 0000 0000 0001


Initial conditions:
1. Reset all nodes
2. Set dutZC under target stack profile, dutZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
nwkExtendedPANID = 0x0000 0000 0000 0001
apsDesignatedCoordinator = TRUE
apsUseExtendedPANID = 0x0000 0000 0000 0001
apsUseInsecureJoin = TRUE
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: 0x abcd ef01 2345 6789 0000 0000 0000 0000
nwkActiveKeySeqNumber: 0x00
apsDeviceKeyPairSet
Key 0: sharedTrustCenterLinkKey = 5A 69 67 42 65 65 41 6C 6C 69 61 6E 63 65 30 39 (ZigBeeAlliance09)
3. gZR is configured with
nwkExtendedPANID = 0x0000 0000 0000 0001
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0001
apsUseInsecureJoin = TRUE
nwkSecurityLevel: 0x05
apsDeviceKeyPairSet.LinkKey:
sharedTrustCenterLinkKey = 5A 69 67 42 65 65 41 6C 6C 69 61 6E 63 65 30 39 (ZigBeeAlliance09)
apsLinkKeyType: 0x01 (shared)
4.dutZED is configured with
nwkExtendedPANID = 0x0000 0000 0000 0001
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0001
apsUseInsecureJoin = TRUE
nwkSecurityLevel: 0x05
apsDeviceKeyPairSet.LinkKey:
sharedTrustCenterLinkKey = 5A 69 67 42 65 65 41 6C 6C 69 61 6E 63 65 30 39 (ZigBeeAlliance09)
apsLinkKeyType: 0x01 (shared)

Test procedure:
1 Join device gZR to dutZC.
2 gZR requests a Trust Center Link Key from Trust Center using APS Request-Key command
3 gZR confirm the unique Trust Center Link Key update
4 Join device gZED to gZR.
5 gZED requests a Trust Center Link Key from Trust Center using APS Request-Key command
6 gZED confirm the unique Trust Center Link Key update
7 Buffer test exchange to verify the new unique key is in place and properly being used

Pass Verdict:
1.	gZR shall issue an MLME Beacon Request MAC command frame, and dutZC shall respond with a beacon.
2.	gZR is able to complete the MAC association sequence with dutZC and gets a new short address, randomly generated.
3.	dutZC transports current Network key to gZR using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	gZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	gZR queries dutZC’s stack revision field by sending a node_desc_req and parsing the mating node_desc_rsp. If stack revision is less than 21, gZR skips the Trust Center link key update. 
6.	gZR requests Trust Center Link Key from dutZC using APS Request-Key (Key Type = 0x04).
7.	dutZC transports unique Trust Center Link Key using APS Transport-Key (Key Type = 0x04) to gZR; APS Transport-Key encrypted with shared link key.
8.	gZR sends an APS Verify Key Command to dutZC with the AES-MMO hash of ‘0x03’ calculated with the unique TC link-key.  The message is not APS encrypted.
9.	dutZC sends an APS Verified Key Command to gZR with status = SUCCESS.  The message is APS encrypted with the unique TC link-key.
10.	gZED is able to complete the MAC association sequence with gZR and gets a new short address, randomly generated.
11.	gZR sends an APS Update-Device command to dutZC with APS level encryption using unique Trust Center Link Key, with a status code of 0x01 (Standard Device unsecured join).
12.	dutZC sends an APS Tunnel command to gZR, containing an APS transport key command that is APS encrypted.
13.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
14.	gZED queries dutZC’s stack revision field by sending a node_desc_req and parsing the mating node_desc_rsp. If stack revision is less than 21, gZED skips the Trust Center link key update. 
15.	gZED requests Trust Center Link Key from dutZC using APS Request-Key (Key Type = 0x04).
16.	dutZC transports unique Trust Center Link Key using APS Transport-Key (Key Type = 0x04) to gZED; APS Transport-Key encrypted with shared link key.
17.	gZED sends an APS Verify Key Command to dutZC with the AES-MMO hash of ‘0x03’ calculated with the unique TC link-key.  The message is not APS encrypted.
18.	dutZC sends an APS Confirm Key Command to gZED with status = SUCCESS.  The message is APS encrypted with the unique TC link-key.
19.	gZED sends an Buffer test request to dutZC; and encrypted at APS level using unique Trust Center Link Key
20.	dutZC sends Buffer test response to gZED, encrypted at APS level using unique Trust Center Link Key

Fail verdict:
1.	gZR does not issue an MLME Beacon Request MAC command frame, or dutZC does not respond with a beacon.
2.	gZR is not able to complete the MAC association sequence with dutZC.
3.	dutZC does not send an Transport-Key (NWK key) command to gZR, or dutZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	gZR does not issue a device announcement.
5.	dutZC does not respond to gZR’s node_desc_req with a proper node_desc_rsp, or stack compliance revision field in server mask flags does not indicate R21+ TC.
6.	gZR does not request Trust Center Link Key from dutZC
7.	dutZC does not transport unique Trust Center Link Key to gZR; or APS Transport-Key is not encrypted with the original link key (either shared global or pre-configured device-specific).
8.	gZR does not send an APS Verify Key Command.
9.	dutZC does not send an APS Verified Key Command, or command is not encrypted at the APS layer or encryption key is not the new unique TC link-key or status is not SUCCESS.
10.	gZED is not able to complete the MAC association sequence with gZR.
11.	gZR does not send an APS Update-Device command to dutZC, or does not use unique Trust center Link Key for APS level encryption, or does not set the status code to 0x01.
12.	dutZC does not send an APS tunnel command to gZR.
13.	gZED does not issue a device announcement.
14.	dutZC does not respond to gZED’s node_desc_req with a proper node_desc_rsp, or stack compliance revision field in server mask flags does not indicate R21+ TC.
15.	gZED does not request Trust Center Link Key from gZC
16.	dutZC does not transport unique Trust Center Link Key to gZED; or APS Transport-Key is not encrypted with the original link key (either shared global or pre-configured device-specific).
17.	gZED does not send an APS Verify Key Command.
18.	dutZC does not send an APS Confirm Key Command, or command is not encrypted at the APS layer or encryption key is not the new unique TC link-key or status is not SUCCESS.
19.	gZED does not send Buffer test request to dutZC; or Buffer test request is not encrypted with unique Trust Center Link Key at APS level
20.	dutZC does not send Buffer test response to gZED, or Buffer test response is not encrypted with unique Trust Center Link Key at APS level
