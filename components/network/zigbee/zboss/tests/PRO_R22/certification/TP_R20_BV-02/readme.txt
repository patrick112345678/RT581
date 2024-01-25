TP/R20//BV-2 Update Device – Global Link Keys - TC (CCB 1313)

Objective: To confirm that the Trust Center will accept an APS update device with or without APS encryption when using a
global link key to that device.
DUT:ZC


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
2. Set DUT ZC under target stack profile, DUT ZC as coordinator starts a secured PAN = 0x1AAA network;
   it is the Trust Center for the PAN
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = TRUE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    nwkSecurityMaterialSet.key: 0x abcd ef01 2345 6789 0000 0000 0000 0000
    nwkActiveKeySeqNumber: 0x00
    apsDeviceKeyPairSet
       Key 0: LinkKey: 0x1233 3333 3333 3333 3333 3333 3333,
          DeviceAddress: 0x 0000 0001 0000 0000,
          apsLinkKeyType: 0x01 (global)
       Key 1: LinkKey: 0x1233 3333 3333 3333 3333 3333 3333,
          DeviceAddress: 0x 0000 0000 0000 0001,
          apsLinkKeyType: 0x01 (global)
3. gZR1 is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)
    apsLinkKeyType: 0x00 (unique)
4. gZED1 is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)
5. gZED2 is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)

Test procedure:
1. Join device gZR1 to DUT ZC.
2. Join device gZED1 to gZR1.
3. Configure gZR1 to send the APS update device without APS encryption.
4. Join device gZED2.  

Pass Verdict
1. gZR1 shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
2. gZR1 is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
3. DUT ZC sends an APS encrypted Transport key to gZR1 with the current value of the Network Key.
4. gZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5. gZED1 shall issue an MLME Beacon Request MAC command frame, and gZR1 shall respond with a beacon.
6. gZED1 is able to complete the MAC association sequence with gZR1 and gets a new short address, randomly generated.
7. gZR1 sends an APS update device command to DUT ZC with APS encryption, with a status code of 0x01 (Standard Device unsecured join).
8. DUT ZC sends an APS Tunnel Data command to gZR1containing an APS transport key command. The APS Tunnel command shall not be
   APS encrypted, but the contained transport key command shall be APS encrypted.
9. gZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
10. gZED2 shall issue an MLME Beacon Request MAC command frame, and gZR1 shall respond with a beacon.
11. gZED2 is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
12. gZR1 sends an APS update device command to DUT ZCwithout APS encryption, with a status
    code of 0x01 (Standard Device unsecured join).
13. DUT ZC sends an APS Tunnel Data command to gZR1, containing an APS transport key command that is APS encrypted.
14. gZED2 issues a ZDO device announcement sent to the broadcast address (0xFFFD).

Fail Verdict
1. gZR1does not issue an MLME Beacon Request MAC command frame, or DUT ZCdoes not respond with a beacon.
2. gZR1 is not able to complete the MAC association sequence.
3. DUT ZC does not send an APS transport key command.
4. gZR1 does not issue a device announcement.
5. gZED1 does not issue an MLME Beacon Request MAC command frame, or gZR1does not respond with a beacon.
6. gZED1 is not able to complete the MAC association sequence.
7. gZR1 does not send an APS update device command, or does not set the status code to 0x01.
8. DUT ZC does not send an APS tunnel data command. The transport key command is not APS encrypted.
9. gZED1 does not issue a device announcement.
10. gZED2 does not issue an MLME Beacon Request MAC command frame, or gZR1does not respond with a beacon.
11. gZED2 is not able to complete the MAC association sequence.
12. gZR1 does not send an APS update device command, or does not set the status code to 0x01.
13. DUT ZC does not send an APS tunnel data command.
14. gZED2 does not issue a device announcement.

Comments:


