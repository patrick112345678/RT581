10.65 TP/R20/BV-11 Support for Default Trust Center Link Key (CCB 1039)
Objective: To confirm that the device can be configured to support the default trust center link key.
DUT: ZC, ZR or ZED

ZC 	EPID = 0x0000 0000 0000 0001
	Long Address = 0x AAAA AAAA AAAA AAAA
	PAN ID = 0x1aaa
	Short Address = 0x0000

ZED or ZR	EPID = 0x0000 0000 0000 0001
		PAN ID = 0x1aaa
		Short Address = Generated in a random manner within the range 1 to 0xFFF7
		Long Address = <device-specific-EUI64>

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
	Key 0: LinkKey: 0x5A69 6742 6565 416C 6C69 616E 6365 3039 (ZigBeeAlliance09),
    DeviceAddress: any
    apsLinkKeyType: 0x01 (global)
3. ZR or ZED is configured with
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x5A69 6742 6565 416C 6C69 616E 6365 3039 (ZigBeeAlliance09),
    apsLinkKeyType: 0x01 (global)

Test Procedure
1. Join device ZR or ZED to ZC

Pass verdict:
1. ZR or ZED shall issue an MLME Beacon Request MAC command frame, and ZC shall respond with a beacon.
2. ZR or ZED is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
3. ZC sends an APS encrypted Transport key to ZR or ZED with the current value of the Network Key.
4. ZR or ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).

Fail verdict:
1. ZR or ZED does not issue an MLME Beacon Request MAC command frame, or ZCdoes not respond with a beacon.
2. ZR or ZED is not able to complete the MAC association sequence.
3. ZC does not send an APS transport key command.
4. ZR or ZED does not issue a device announcement.

Comments:
Use runng_ed.sh  in case with ZED;
Use runng_r.sh in case with ZR;
