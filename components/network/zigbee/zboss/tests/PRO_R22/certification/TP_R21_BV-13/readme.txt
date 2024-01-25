TP/R21/BV-13 Resetting NWK Security Outgoing frame counter and NWK Security Incoming frame counter maintenance

Objective: Reset the NWK Security outgoing frame counter upon Key update when outgoing frame counter
           is higher than the 0x800000000. This also ensures the separate Incoming frame counter per NWK key.

Initial Conditions:

       gZC1
  |          |
  |          |
DUT_ZR1    DUT_ZR2
  |
  |
DUT_ZED


Test Procedure:
1 Join device dutZR1 to gZC. (Criteria 1 - 4)
2 Join device dutZR2 to gZC. (Criteria 5 - 8)
3 Join device dutZED to dutZR1. (Criteria 9 - 13)
4 The dutZR1 sends a broadcast buffer test request using nwkProtocol = 0x02 (Zigbee Pro). (Criteria 14 - 15)
5 Set the NWK Security Outgoing frame counter as 0x80000001 in dutZR1, by implementation specific way (Criteria 16)
6 The dutZR1 sends a broadcast buffer test request using nwkProtocol = 0x02 (Zigbee Pro). (Criteria 17 - 18)
7 gZC broadcasts NWK Key using Transport-Key command (Criteria 19)
8 gZC broadcasts Switch-Key command (Criteria 20)
9 The dutZR1 sends a broadcast buffer test request using nwkProtocol = 0x02 (Zigbee Pro). (Criteria 21 - 22)


Pass Verdict:
1 dutZR1 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 dutZR1 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3 gZC transports current Network key to dutZR1 using APS Transport-Key; APS Transport-Key encrypted with shared link
  key at APS level.
4 dutZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 dutZR2 shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
6 dutZR2 is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
7 gZC transports current Network key to dutZR2 using APS Transport-Key; APS Transport-Key encrypted with shared link
  key at APS level.
8 dutZR2 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
9 gZED shall issue an MLME Beacon Request MAC command frame, and dutZR1 shall respond with a beacon.
10 gZED is able to complete the MAC association sequence with dutZR1 and gets a new short address, randomly generated.
11 dutZR1 sends an APS Update-Device command to gZC with APS level encryption using shared Trust Center Link Key,
   with a status code of 0x01 (Standard Device unsecured join).
12 gZC sends an APS Tunnel command to dutZR1, containing an APS transport key command that is APS encrypted.
13 gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
14 The dutZR1 sends a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro).
15 gZC, dutZR2 and dutZED respond to the buffer test request message with buffer test response
16 dutZR1 updates its NWK Security outgoing frame counter as 0x80000001
17 The dutZR1 sends a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro).
18 gZC, dutZR2 and dutZED respond to the buffer test request message with buffer test response
19 gZC broadcasts an APS Transport Key command with key type 0x01 (Standard Network Key) without APS encryption.
   The new NWK key shall be the Key1 as defined in the test setup.
20 gZC broadcasts an APS Switch-Key command
21 The dutZR1 sends a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro);
   And its NWK Security Outgoing frame counter reset to 0x00 00 00 00 00.
22 gZC, dutZR2 and dutZED respond to the buffer test request message with buffer test response

Fail Cerdict:
1 dutZR1 does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2 dutZR1 is not able to complete the MAC association sequence with gZC.
3 gZC does not send an Transport-Key (NWK key) command to dutZR1, or gZC does not encrypt the Transport-Key
  (NWK key) at APS level.
4 dutZR1 does not issue a device announcement.
5 dutZR2 does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
6 dutZR2 is not able to complete the MAC association sequence with gZC.
7 gZC does not send an Transport-Key (NWK key) command to dutZR2, or gZC does not encrypt the Transport-Key
  (NWK key) at APS level.
8 dutZR2 does not issue a device announcement.
9 gZED does not issue an MLME Beacon Request MAC command frame, or dutZR1 does not respond with a beacon.
10 gZED is not able to complete the MAC association sequence with dutZR1.
11 dutZR1 does not send an APS Update-Device command to gZC, or does not use shared Trust center Link Key for APS level encryption,
   or does not set the status code to 0x01.
12 gZC does not send an APS tunnel command to dutZR1.
13 gZED does not issue a device announcement.
14 The dutZR1 does not send a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro).
15 gZC or dutZR2 or dutZED does not respond to the buffer test request message with buffer test response
16 dutZR1 does not update its NWK Security outgoing frame counter as 0x80000001
17 The dutZR1 does not send a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro).
18 gZC or dutZR2 or dutZED does not respond to the buffer test request message with buffer test response
19 gZC does not broadcast the new NWK key.
20 gZC does not broadcast an APS Switch-Key command
21 The dutZR1 does not send a broadcast (0xFFFF) buffer test request using nwkProtocol = 0x02 (Zigbee Pro);
   or its NWK Security Outgoing frame counter does not reset to 0x00 00 00 00.
22 gZC or dutZR2 or dutZED does not respond to the buffer test request message with buffer test response

Additional info:
 - Delay between test start and test launching is 30 seconds: devices exchanges their Link Statuses.
