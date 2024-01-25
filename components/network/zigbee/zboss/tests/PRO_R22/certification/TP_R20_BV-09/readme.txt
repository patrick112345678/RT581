10.63 TP/R20/BV-09  Secure Rejoin – Global Link Keys
(CCB 1313)
Objective:  To confirm that a router will send an APS Update Device command without APS encryption when using a global link key; to confirm that the rejoining device will successfully get back on the network with a secured rejoin; to confirm that the ZC does not send a NWK key to the rejoining device since it already has the NWK key.
DUT:  ZC, ZR1 or ZED1

ZC	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = 0x0000
	Long Address = 0x AAAA AAAA AAAA AAAA
ZR1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0001 0000 0000
ZED1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0001

Initial condition:
1 Reset all nodes
2 Set ZC under target stack profile, ZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
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
3 ZR1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)
4 ZED1 is configured with 
    nwkExtendedPANID = 0x0000 0000 0000 0001
    apsDesignatedCoordinator = FALSE
    apsUseExtendedPANID = 0x0000 0000 0000 0001
    apsUseInsecureJoin = TRUE
    nwkSecurityLevel: 0x05
    apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
    apsLinkKeyType: 0x01 (global)

Test procedure:
1 Join device ZR1 to ZC.
2 Join device gZED1 to ZR1.
3 Rejoin device ZED1 using secure rejoin using device specific mechanism

Criteria
Pass Verdict
1 ZR1 shall issue an MLME Beacon Request MAC command frame, and ZC shall respond with a beacon.
2 ZR1 is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
3 ZC sends an APS encrypted Transport key to ZR1 with the current value of the Network Key.
4 ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 ZED1 shall issue an MLME Beacon Request MAC command frame, and ZR1 shall respond with a beacon.
6 ZED1 is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
7 ZR1 sends an APS update device command to ZC with APS encryption.  ZR1 may also send an APS update without APS encryption either before or after the APS update device command with APS encryption.  The status code of the update device shall be 0x01 (Standard Device unsecured join).
8 ZC sends an APS Tunnel Data command to ZR1, containing an APS transport key command that is APS encrypted.
ZC may respond twice with 2 APS tunnel data commands if the ZR1 issued 2 update device commands in the previous step.
9 ZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
10 ZED1 issues a NWK encrypted Rejoin Request to ZR1.  The rejoin request shall include the source IEEE address sub-field in the NWK header.
11 ZR1 issues a NWK encrypted Rejoin Response to ZED1 with status of 0x00 (success).
12 ZR1 sends an update device command to the ZC with APS encryption.  ZR1 may also send an APS update without APS encryption either before or after the APS update device command with APS encryption.  The status code shall be 0x00 (Standard device secured rejoin).
13 ZC shall send no transport key or tunnel command messages.

Fail Verdict
1 ZR1 does not issue an MLME Beacon Request MAC command frame, or ZC does not respond with a beacon.
2 ZR1 is not able to complete the MAC association sequence.
3 ZC does not send an APS transport key command.
4 ZR1 does not issue a device announcement.
5 ZED1 does not issue an MLME Beacon Request MAC command frame, or ZR1 does not respond with a beacon.
6 ZED1 is not able to complete the MAC association sequence.
7 ZR1 does not send an APS update device command, or does not set the status code of the update device to 0x01.
8 ZC does not send an APS tunnel data command.
9 ZED1 does not issue a device announcement.
10 ZED1 does not issue a NWK encrypted rejoin request, or the rejoin request does not include the source IEEE address sub-field.
11 ZR1 does not issue a NWK encrypted rejoin response.
12 ZR1 does not issue an APS update device, or does not set the status code to 0x00.
13 ZC sends a tunnel command or transport key message.

Comments:



