TP/R20/BV-05 APS Remove Device Processing – Unique Link Keys
(CCB 1280)
Objective:  To confirm that a router will accept and process an APS remove device command but only for APS encrypted messages. 
DUT:  ZR1

gZC	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = 0x0000
	Long Address = 0x AAAA AAAA AAAA AAAA
DUT ZR1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0001 0000 0000
gZED	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0001

Initial conditions:
1.Reset all nodes
2.Set gZC under target stack profile, gZC as coordinator starts a secured PAN = 0x1AAA network;
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
      apsLinkKeyType: 0x00 (unique)
   Key 1: LinkKey: 0x4566 6666 6666 6666 6666 6666 6666,
      DeviceAddress: 0x 0000 0000 0000 0001,
      apsLinkKeyType: 0x00 (unique)
3.DUT ZR1 is configured with
   nwkExtendedPANID = 0x0000 0000 0000 0001
   apsDesignatedCoordinator = FALSE
   apsUseExtendedPANID = 0x0000 0000 0000 0001
   apsUseInsecureJoin = TRUE
   nwkSecurityLevel: 0x05
   apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
   apsLinkKeyType: 0x00 (unique)
4.gZED is configured with
   nwkExtendedPANID = 0x0000 0000 0000 0001
   apsDesignatedCoordinator = FALSE
   apsUseExtendedPANID = 0x0000 0000 0000 0001
   apsUseInsecureJoin = TRUE
   nwkSecurityLevel: 0x05
   apsDeviceKeyPairSet.LinkKey: 0x4566 6666 6666 6666 6666 6666 6666
   apsLinkKeyType: 0x00 (unique)
   If gZED is a polling device, set the rate to 3 seconds. See the Notes section at the
   end of this test case for more details.

Test procedure:
1. Join device DUT ZR1 to gZC.
2. Join device gZED to DUT ZR1.
3. gZC sends an APS remove device command to DUT ZR1 with targetAddress = 0x 0000 0000 0000 0001  (gZED) without APS encryption.
4. gZC sends an APS remove device command to the broadcast address 0xFFFC (‘all routers and coordinator’)
   with targetAddress = 0x0000 0000 0000 0001 (gZED) with APS encryption, unsing a suitable link key
   (one that DUT ZR1 is able to decrypt).
   Note: This test can be omitted, if gZC is not capable of sending remove device as a broadcast.
5. gZC sends an APS remove device command to DUT ZR1 with targetAddress of EUI64 of gZED
   (0x 0000 0000 0000 0001, with APS encryption.
6. gZC sends an APS remove device command to DUT ZR1 with targetAddress of EUI64 of gZR1, with APS encryption.


Pass Verdict
1 DUT ZR1 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 DUT ZR1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3 gZC sends an APS encrypted Transport key to DUT ZR1 with the current value of the Network Key.
4 DUT ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 gZED shall issue an MLME Beacon Request MAC command frame, and DUT ZR1 shall respond with a beacon.
6 gZED is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
7 DUT ZR1 sends an APS update device command to gZC with APS encryption.
8 gZC sends an APS Tunnel Data command to DUT ZR1, containing an APS transport key command that is APS encrypted.
9 gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
10 gZC sends an APS remove device without APS encryption to DUT ZR1. The target address in the message shall
   be the EUI64 of gZED1.
11 DUT ZR1 does not remove gZED from its child table and does not send a NWK leave to gZED.
   It does not remove itself from the network.
12 DUT ZR1 does not remove gZED from its child table and does not send a NWK leave to gZED.
   It does not remove itself from the network.
13 gZC sends an APS remove device with APS encryption to DUT ZR1.
14 DUT ZR1 sends a NWK Leave request command to gZED, and removes the device from its child table.
   gZED leaves the network.
14.a DUT ZR1 sends an update device (status = ‘device left’) to gZC once it receives the leave indication from gZED.
15 gZC sends an APS remove device with APS encryption to DUT ZR1.
16 DUT ZR1 sends a NWK Leave announcement for itself. DUT ZR1 leaves the network.

Fail Verdict
1 DUT ZR1 does not issue an MLME Beacon Request MAC command frame, or gZC does notrespond with a beacon.
2 DUT ZR1 is not able to complete the MAC association sequence.
3 gZC does not send an APS transport key command.
4 DUT ZR1 does not issue a device announcement.
5 gZED does not issue an MLME Beacon Request MAC command frame, or DUT ZR1 does not respond with a beacon.
6 gZED is not able to complete the MAC association sequence.
7 DUT ZR1 does not send an APS update device command.
8 gZC does not send an APS tunnel data command.
9 gZED does not issue a device announcement.
10 gZC does not send an APS remove device command.
11 DUT ZR1 issues a NWK leave command, either a request or an announcement.
12 DUT ZR1 emits a NWK leave command, either a request or an announcement.
13 gZC does not send an APS remove device command.
14 DUT ZR1 does not send a NWK leave request.
14.a DUT ZR1 does not send an update device (status = ‘device left’) to gZC.
15 gZC does not send an APS remove device command.
16 DUT ZR1 does not send a NWK leave announcement. It does not leave the network.

Comments: In order to verify that the ZED has left the network, the test house needs an observable behaviour.
If gZED does not send out NWK leave announcements when it is told to leave, then polling should be turned on for
the end device throughout the test. Leave announcements are only required for routers in the current version of
the specification. If gZED doesn’t send leave announcements, then the polling can be used as an observable test
to determine when it has left the network. Polling in this case should be done at a 3 second rate.
If the gZED sends out a leave announcement, it isn’t necessary to poll. It may be an RxOnWhenIdle=TRUE device.





