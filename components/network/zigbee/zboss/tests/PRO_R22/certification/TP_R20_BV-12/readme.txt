10.66 TP/R20/BV-12 Commissioning the Short Address (CCB1312)
Objective: To confirm that the device can be commissioned to use a predefined shortaddress. DUT: ZR or ZED

gZC: 	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = 0x0000
	Long Address = 0x AAAA AAAA AAAA AAAA
DUT ZR or DUT ZED 	EPID = 0x0000 0000 0000 0001
			PAN ID = 0x1aaa
			Short Address = 0xABCD
			Long Address = 0x 0000 0001 0000 0000

Initial conditions:
1. Reset all nodes
2. Set gZC under target stack profile, gZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = TRUE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    nwkSecurityMaterialSet.key: 0x abcd ef01 2345 6789 0000 0000 0000 0000
    nwkActiveKeySeqNumber: 0x00
    apsDeviceKeyPairSet
    Key 0: LinkKey: 0x5A69 6742 6565 416C 6C69 616E 6365 3039 (ZigBeeAlliance09),
    DeviceAddress: 0x 0000 0001 0000 0000,
    apsLinkKeyType: 0x01 (global)
    Key 1: LinkKey: 0x5A69 6742 6565 416C 6C69 616E 6365 3039 (ZigBeeAlliance09),
    DeviceAddress: 0x 0000 0000 0000 0001, apsLinkKeyType: 0x01 (global)
3. ZR or ZED is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    nwkSecurityMaterialSet.key: 0x abcd ef01 2345 6789 0000 0000 0000 0000
    nwkActiveKeySeqNumber: 0x00
    apsDeviceKeyPairSet.LinkKey: 0x5A69 6742 6565 416C 6C69 616E 6365 3039 (ZigBeeAlliance09),
    apsLinkKeyType: 0x01 (global)

Test procedure:
1. Commission device ZR or ZED to use a short address of 0xABCD.
2. Join device ZR or ZED to using Trust Center Rejoin gZC.

Pass Verdict
1. ZR or ZED shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2. ZR or ZED shall send an unencrypted NWK Rejoin Request Command to gZC using a short ID of 0xABCD.
3. gZC shall send an unencrypted NWK Rejoin Response command to ZR or ZED with the NWK
destination set to the device’s commissioned short address of 0xABCD.
4. gZC sends an APS encrypted Transport key to ZR or ZED with the current value of the Network Key.
5. ZR or ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).

Fail Verdict
1. ZR or ZED does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2. ZR or ZED does not send an unencrypted NWK Rejoin Request command.
3. gZCdoes not send the NWK rejoin response command to ZR or ZED.
4. gZC does not send an APS transport key command.
5. ZR or ZED does not issue a device announcement.

Comments:



