TP/R21/BV-12 Outgoing NWK frame-counter persistence
Objective: Make sure that DUT ZR/ZED keeps its outgoing frame-counter for NWK security across simple factory
resets (in particular when leaving a network);

Initial Conditions:

      gZC1
  |          |
  |          |
DUT_ZR     DUT_ZED

      gZC2
  |          |
  |          |
DUT_ZR     DUT_ZED

Test Procedure:
1 Join device dutZR to gZC1 in network1. (Criteria 1-4)
2 Join device dutZED to gZC1 in network1. Criteria 5-8)
3 Factory-Reset dutZED. (implementation specific way) (Criteria 9)
4 Factory-Reset dutZR. (implementation specific way) (Criteria 10)
5 Join device dutZR to gZC2 in network 2. (Criteria 11 - 15)
  Notice: It might be necessary to revert to the default key, if dutZR has updated its TC link key.
6 Join device dutZED to gZC2 in network 2. (Criteria 16 - 20)
  Notice: It might be necessary to revert to the default key, if dutZR has updated its TC link key.
7 Factory-Reset dutZED. (implementation specific way) (Criteria 21)
8 Factory-Reset dutZR. (implementation specific way) (Criteria 22)
9a If gZC1’s trust center policies require explicit removal of automatically generated unique trust center
   link-keys, erase the entry for dutZR from gZC1’s apsDeviceKeyPairSet table.
9b Join device dutZR to gZC1 in network 1, again. (Criteria 23 – 27)
   Notice: It might be necessary to revert to the default key, if dutZR has updated its TC link key.
10a If gZC1’s trust center policies require explicit removal of automatically generated unique trust
    center link-keys, erase the entry for dutZED from gZC1’s apsDeviceKeyPairSet table.
10b Join device dutZED to gZC1 in network 1, again. (Criteria 28 - 32)
    Notice: It might be necessary to revert to the default key, if dutZR has updated its TC link key.


Pass Verdict:
1 dutZR shall issue an MLME Beacon Request MAC command frame, and gZC1 shall respond with a beacon.
2 dutZR is able to complete the MAC association sequence with gZC1 and gets a new short address, randomly generated.
3 gZC1 transports current Network key to dutZR using APS Transport-Key; APS Transport-Key encrypted with shared
   link key at APS level.
4 dutZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 dutZED shall issue an MLME Beacon Request MAC command frame, and gZC1 shall respond with a beacon.
6 dutZED is able to complete the MAC association sequence with gZC1 and gets a new short address, randomly generated.
7 gZC1 transports current Network key to dutZED using APS Transport-Key; APS Transport-Key encrypted with shared
   link key at APS level.
8 dutZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
9 dutZED successfully reset to Factory New; all NWK and Security parameters are reset, except NWK security frame counter
10 dutZR successfully reset to Factory New; all NWK and Security parameters are reset, except NWK security frame counter
11 dutZR shall issue an MLME Beacon Request MAC command frame, and gZC2 shall respond with a beacon.
12 dutZR is able to complete the MAC association sequence with gZC2 and gets a new short address, randomly generated.
13 gZC2 transports current Network key to dutZR using APS Transport-Key; APS Transport-Key encrypted with
    shared link key at APS level.
14 dutZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
15 dutZR’s NWK security frame counter is higher than the NWK security frame counter it had in network1
16 dutZED shall issue an MLME Beacon Request MAC command frame, and gZC2 shall respond with a beacon.
17 dutZED is able to complete the MAC association sequence with gZC2 and gets a new short address, randomly generated.
18 gZC2 transports current Network key to dutZED using APS Transport-Key; APS Transport-Key encrypted with shared
    link key at APS level.
19 dutZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
20 dutZED’s NWK security frame counter is higher than the NWK security frame counter it had in network1
21 dutZED successfully reset to Factory New; all NWK and Security parameters are reset, except NWK security frame counter
22 dutZR successfully reset to Factory New; all NWK and Security parameters are reset, except NWK security frame counter
23 dutZR shall issue an MLME Beacon Request MAC command frame, and gZC1 shall respond with a beacon.
24 dutZR is able to complete the MAC association sequence with gZC1 and gets a new short address, randomly generated.
25 gZC1 transports current Network key to dutZR using APS Transport-Key; APS Transport-Key encrypted with
    shared link key at APS level.
26 dutZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
27 dutZR’s NWK security frame counter is higher than the NWK security frame counter it had in network2.
28 dutZED shall issue an MLME Beacon Request MAC command frame, and gZC1 shall respond with a beacon.
29 dutZED is able to complete the MAC association sequence with gZC1 and gets a new short address, randomly generated.
30 gZC1 transports current Network key to dutZED using APS Transport-Key; APS Transport-Key encrypted with shared
    link key at APS level.
31 dutZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
32 dutZED’s NWK security frame counter is higher than the NWK security frame counter it had in network2.


Fail Verdict:
1 dutZR does not issue an MLME Beacon Request MAC command frame, or gZC1 does not respond with a beacon.
2 dutZR is not able to complete the MAC association sequence with gZC1.
3 gZC1 does not send an Transport-Key (NWK key) command to dutZR, or gZC1 does not encrypt the
  Transport-Key (NWK key) at APS level.
4 dutZR does not issue a device announcement.
5 dutZED does not issue an MLME Beacon Request MAC command frame, or gZC1 does not respond with a beacon.
6 dutZED is not able to complete the MAC association sequence with gZC1.
7 gZC1 does not send an Transport-Key (NWK key) command to dutZED, or gZC1 does not encrypt the
  Transport-Key (NWK key) at APS level.
8 dutZED does not issue a device announcement.
9 dutZED does not rest to Factory New; or NWK and Security parameters are not reset; or NWK security frame counter is reset
10 dutZR does not rest to Factory New; NWK and Security parameters are not reset; or NWK security frame counter is reset
11 dutZR does not issue an MLME Beacon Request MAC command frame, or gZC2 does not respond with a beacon.
12 dutZR is not able to complete the MAC association sequence with gZC2.
13 gZC2 does not send an Transport-Key (NWK key) command to dutZR, or gZC2 does not encrypt the Transport-Key
   (NWK key) at APS level.
14 dutZR does not issue a device announcement.
15 dutZR’s NWK security frame counter is not higher than the NWK security frame counter it had in network1
16 dutZED does not issue an MLME Beacon Request MAC command frame, or gZC2 does not respond with a beacon.
17 dutZED is not able to complete the MAC association sequence with gZC2.
18 gZC2 does not send an Transport-Key (NWK key) command to dutZED, or gZC2 does not encrypt the
   Transport-Key (NWK key) at APS level.
19 dutZED does not issue a device announcement.
20 dutZED’s NWK security frame counter is not higher than the NWK security frame counter it had in network1
21 dutZED does not rest to Factory New; or NWK and Security parameters are not reset; or NWK security frame counter is reset
22 dutZR does not rest to Factory New; NWK and Security parameters are not reset; or NWK security frame counter is reset
23 dutZR does not issue an MLME Beacon Request MAC command frame, or gZC1 does not respond with a beacon.
24 dutZR is not able to complete the MAC association sequence with gZC1.
25 gZC1 does not send an Transport-Key (NWK key) command to dutZR, or gZC1 does not encrypt the Transport-Key (NWK key) at APS level.
26 dutZR does not issue a device announcement.
27 dutZR’s NWK security frame counter is not higher than the NWK security frame counter it had in network2.
28 dutZED does not issue an MLME Beacon Request MAC command frame, or gZC1 does not respond with a beacon.
29 dutZED is not able to complete the MAC association sequence with gZC
30 gZC1 does not send an Transport-Key (NWK key) command to dutZED, or gZC1 does not encrypt the Transport-Key
   (NWK key) at APS level.
31 dutZED does not issue a device announcement.
32 dutZED’s NWK security frame counter is not higher than the NWK security frame counter it had in network2.

