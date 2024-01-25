TP/R20/TP/BV-01 Update Device – Unique Link Keys (CCB 1313)
Objective: To confirm that a router will send an APS Update Device command with APS encryption when using a unique link key. To confirm that the Trust Center will reject an APS update device without APS encryption when using a unique link key.
DUT: ZR1 or ZC


    ZC
    |
    ZR
  |    |
 ZED1 ZED2

ZC	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = 0x0000
	Long Address = 0x AAAA AAAA AAAA AAAA
ZR1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0001 0000 0000
gZED1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0001
gZED2	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0002

Initial conditions:
1. Reset all nodes
2. Set ZC under target stack profile, ZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = TRUE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    nwkSecurityMaterialSet.key: 0x abcd ef01 2345 6789 0000 0000 0000 0000
    nwkActiveKeySeqNumber: 0x00
    apsDeviceKeyPairSet
	Key 0: LinkKey: 0x1233 3333 3333 3333 3333 3333 3333, 
	DeviceAddress:  0x 0000 0001 0000 0000, 
    apsLinkKeyType: 0x00 (unique)
    Key 1: LinkKey: 0x4566 6666 6666 6666 6666 6666 6666, 
    DeviceAddress:  0x 0000 0000 0000 0001,
    apsLinkKeyType: 0x00 (unique)
3. ZR1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x00 (unique)
4. gZED1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x4566 6666 6666 6666 6666 6666 6666
    apsLinkKeyType: 0x00 (unique)
5. gZED2 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
(NOTE:  Security material is not necessary for gZED2.  It is expected that gZED2 will not successfully join the network.)

Test procedure:
1. Join device ZR1 to ZC.
2. Join device gZED1 to ZR1.
3. Configure ZR1 to send the APS update device without APS encryption.  This step shall be skipped when running with ZR1 as DUT.
4. Join device gZED2.  This step shall be skipped when running with ZR1 as DUT.

Pass Verdict
1. ZR1 shall issue an MLME Beacon Request MAC command frame, and ZC shall respond with a beacon.
2. ZR1 is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
3. ZC sends an APS encrypted Transport key to ZR1 with the current value of the Network Key.
4. ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5. gZED1 shall issue an MLME Beacon Request MAC command frame, and ZR1 shall respond with a beacon.
6. gZED1 is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
7. ZR1 sends an APS update device command to ZC with APS encryption, with a status code of 0x01 (Standard Device unsecured join).
8. ZC sends an APS Tunnel Data command to ZR1containing an APS transport key command.  The APS Tunnel command shall not be APS encrypted, but the contained transport key command shall be APS encrypted.
9. gZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
10. gZED2 shall issue an MLME Beacon Request MAC command frame, and ZR1 shall respond with a beacon.
11. gZED2 is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
12. ZR1 sends an APS update device command to ZC without APS encryption.
13. ZC sends no APS command to ZR1.

Fail Verdict
1. ZR1 does not issue an MLME Beacon Request MAC command frame, or ZC does not respond with a beacon.
2. ZR1 is not able to complete the MAC association sequence.
3. ZC does not send an APS transport key command.
4. ZR1 does not issue a device announcement.
5. gZED1does not issue an MLME Beacon Request MAC command frame, or ZR1does not respond with a beacon.
6. gZED1 is not able to complete the MAC association sequence.
7. ZR1 does not send an APS update device command, or does not set the status code to 0x01.
8. ZC does not send an APS tunnel data command.  The transport key command is not APS encrypted.
9. gZED1 does not issue a device announcement.
10. gZED2does not issue an MLME Beacon Request MAC command frame, or ZR1 does not respond with a beacon.
11. gZED2 is not able to complete the MAC association sequence.
12. ZR1 does not send an APS update device command.
13. ZC sends an APS command to ZR1.

Comments:


