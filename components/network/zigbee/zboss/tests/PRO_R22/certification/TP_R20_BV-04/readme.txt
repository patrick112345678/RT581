10.58 TP/R20/BV-04 Update Device – Global Link Keys – ZR talking to legacy ZC that requires NO APS encryption
(CCB 1313)
Objective:  To confirm that the ZR sends an APS update device without APS encryption command when talking to a legacy ZC that requires NO APS encryption on that command.  
DUT:  ZR1

gZC	EPID = 0x0000 0000 0000 0001
        PAN ID = 0x1aaa
	Short Address = 0x0000
        Long Address = 0x AAAA AAAA AAAA AAAA
DUT ZR1 EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
        Short Address = Generated in a random manner within the range 1 to 0xFFF7
        Long Address = 0x 0000 0001 0000 0000
gZED1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0001

Initial conditions:
1.Reset all nodes
2. Set gZC under target stack profile, gZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
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
    apsLinkKeyType: 0x01 (global)
    Key 1: LinkKey: 0x1233 3333 3333 3333 3333 3333 3333, 
    DeviceAddress:  0x 0000 0000 0000 0001,
    apsLinkKeyType: 0x01 (global)
3.DUT ZR1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)
4.gZED1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)


Test procedure:
1.Configure gZC to only accept APS Update device commands without APS encryption, simulating a legacy device.
2.Join device DUT ZR1 to gZC.
3.Join device gZED1 to DUT ZR1.

Criteria
Pass Verdict
1. DUT ZR1 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2. DUT ZR1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3. gZC sends an APS encrypted Transport key to DUT ZR1 with the current value of the Network Key.
4. DUT ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5. gZED1 shall issue an MLME Beacon Request MAC command frame, and DUT ZR1 shall respond with a beacon.
6. gZED1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
7. DUT ZR1 sends an APS update device command to gZC without APS encryption.
DUT ZR1 may also send an APS update with APS encryption either before or after the APS update device command without APS encryption.  The status code of the APS update device in both cases shall be 0x01 (Standard Device unsecured join).
8. gZC sends an APS Tunnel Data command to DUT ZR1, containing an APS transport key command.  The APS Tunnel command shall not be APS encrypted, but the contained transport key command shall be APS encrypted.
9. gZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).

Fail Verdict
1. DUT ZR1 does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2. DUT ZR1 is not able to complete the MAC association sequence.
3. gZC does not send an APS transport key command.
4. DUT ZR1 does not issue a device announcement.
5. gZED1 does not issue an MLME Beacon Request MAC command frame, or DUT ZR1 does not respond with a beacon.
6. gZED1 is not able to complete the MAC association sequence.
7. DUT ZR1 does not send an APS update device command with APS encryption, or does not set the status code to 0x01.
8. gZC does not send an APS tunnel data command.  The Transport Key command is not APS encrypted.
9. gZED1 does not issue a device announcement.








