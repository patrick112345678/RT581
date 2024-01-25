10.60 Remove Device Processing – Global Link Keys
(CCB1280)
Objective:  To confirm that a router correctly processes an APS remove device sent to it for its child or itself.
DUT:  ZR1
Requirements: 1. gZED1 is a sleepy end device.  2. The Test Profile and clusters detailed in item #2 of the Test Procedure are available on the DUT.

gZC	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = 0x0000
	Long Address = 0x AAAA AAAA AAAA AAAA
DUT ZR1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0001 0000 0000
gZED1	EPID = 0x0000 0000 0000 0001
	PAN ID = 0x1aaa
	Short Address = Generated in a random manner within the range 1 to 0xFFF7
	Long Address = 0x 0000 0000 0000 0001

Initial conditions:
1 Reset all nodes
2 Set gZC under target stack profile, gZC as coordinator starts a secured PAN = 0x1AAA network; it is the Trust Center for the PAN
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
3 DUT ZR1 is configured with 
nwkExtendedPANID = 0x0000 0000 0000 0001
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0001
apsUseInsecureJoin = TRUE
nwkSecurityLevel: 0x05
apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
apsLinkKeyType: 0x01 (global)
4 gZED1 is configured with 
nwkExtendedPANID = 0x0000 0000 0000 0001
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0001
apsUseInsecureJoin = TRUE
nwkSecurityLevel: 0x05
apsDeviceKeyPairSet.LinkKey: 0x1233 3333 3333 3333 3333 3333 3333
apsLinkKeyType: 0x01 (global)
If gZED is a polling device, set the rate to 3 seconds.  See the Notes section at the end of this test case for more details.

Test procedure:
1 Configure gZC to only accept APS encrypted Update device commands, simulating a legacy device.
2 Join device DUT ZR1 to gZC.
3 Join device gZED1 to DUT ZR1.
4 gZED1 shall issue MAC data poll commands at 3 second intervals.
5 gZC shall issue an APS Remove device command to DUT ZR1 with target address of EUI64 of gZED1. 
6 gZC shall issue an APS Remove device command to DUT ZR1 with target address of EUI64 of DUT ZR1.

Criteria
Pass Verdict
1 DUT ZR1 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 DUT ZR1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3 gZC sends an APS encrypted Transport key to DUT ZR1 with the current value of the Network Key.
4 DUT ZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 gZED1 shall issue an MLME Beacon Request MAC command frame, and DUT ZR1 shall respond with a beacon.
6 gZED1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
7 DUT ZR1 sends an APS update device command to gZC with APS encryption.
  DUT ZR1 may also send an APS update without APS encryption either before or after the APS update device
  command with APS encryption. 
  The status code of the APS update device shall be 0x01 (Standard Device unsecured join).
8 gZC sends an APS Tunnel Data command to DUT ZR1 containing an APS transport key command.
  The APS Tunnel command shall not be APS encrypted,
  but the contained transport key command shall be APS encrypted.
  gZC may respond twice with 2 APS tunnel data commands if the DUT ZR1 issued 2 update device commands in the previous step.
9 gZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
10 gZC sends an APS Remove Device command with APS encryption to DUT ZR1.
   The target address in the message shall be the EUI64 of gZED1.
11 Upon receiving the MAC data poll by gZED1, DUT ZR1 shall send a NWK leave command to the
   gZED1 with command Options = 0x40 (Request = true).
12 gZED1 issues a NWK leave command broadcast to the Network Layer destination address 0xFFFD (rxOnWhenIdle = true) 
with command options set to 0x00 (request = false).  The MAC layer of the message shall be a unicast from gZED1 to DUT ZR1.
12.a DUT ZR1 sends an update device (status = ‘device left’) to gZC once it receives the leave indication from gZED1.
13 gZED1 is no longer on the network and does not issue a MAC data poll.
14 gZC sends an APS Remove Device command with APS encryption to DUT ZR1.
   The target address in the message shall be the EUI64 of DUT ZR1.
15 DUT ZR1 issues a NWK leave command broadcast to Network Layer destination address 0xFFFD (rxOnWhenIdle = true) with command options set to 0x00 (request = false). 
The MAC Layer of the message shall be broadcast from DUT ZR1 to 0xFFFF.
16 DUT ZR1 is no longer accessible on the network.

Fail Verdict
1 DUT ZR1 does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2 DUT ZR1 is not able to complete the MAC association sequence.
3 gZC does not send an APS transport key command.
4 DUT ZR1 does not issue a device announcement.
5 gZED1does not issue an MLME Beacon Request MAC command frame, or DUT ZR1 does not respond with a beacon.
6 gZED1 is not able to complete the MAC association sequence.
7 DUT ZR1 does not send an APS update device command with APS encryption, or does not set the status code to 0x01.
8 gZC does not send an APS tunnel data command.  The Transport Key command is not APS encrypted.
9 gZED1 does not issue a device announcement.
10 gZC does not send an APS Remove device command or it is not APS encrypted.
11 gZED1 does not send a MAC data poll, or DUT ZR1 does not send a NWK leave command to gZED1.
12 gZED1 does not issue a NWK leave command with the correct command options or to the correct addresses. 
12.a DUT ZR1 does not send an update device (status = ‘device left’) to gZC.
13 gZED1 issues a MAC data poll or is still connected to the network.
14 gZC does not send an APS Remove device command or it is not APS encrypted.
15 DUT ZR1 does not issue a NWK leave command with the correct command options or to the broadcast address.
16 DUT ZR1 remains on the network.

Notes:
In order to verify that the ZED has left the network, the test house needs an observable behaviour.  
If gZED does not send out NWK leave announcements when it is told to leave, then polling
should be turned on for the end device throughout the test. 
 Leave announcements are only required for routers in the current version of the specification.  
If gZED doesn’t send leave announcements, then the polling can be used as an observable test to determine when it has left the network.  
Polling in this case should be done at a 3 second rate.  

If the gZED sends out a leave announcement, it isn’t necessary to poll. It may be an RxOnWhenIdle=TRUE device.


Comments:
