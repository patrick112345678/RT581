TP/R21/BV-09: trust Center Link-Key Update - for ZR/ZED, R21+ TC
Objective: DUT ZR/ZED joins centralized security network formed by gZC (R21+) and request Trust Center Link Key (TCLK) from TC;
           and ZR/ZED verifies TCLK.

Initial conditions:

gZC - DUR ZR - DUT ZED

gZC:
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = 0x0000
Long Address =
0x AAAA AAAA AAAA AAAA

dutZR:
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address =
0x 0000 0001 0000 0000

dutZED:
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address =
0x 0000 0000 0000 0001



Test procedure:
1 Join device dutZR to gZC. (Criteria 1-4)
2 dutZR requests a Trust Center Link Key from Trust Center using APS Request-Key command (Criteria 5-7)
3 dutZR confirms the latest unique Trust Center Link Key update (Criteria 8-9)
4 Join device dutZED to dutZR. (Criteria 10-14)
5 dutZED requests a Trust Center Link Key from Trust Center using APS Request-Key command (Criteria 14-15)
6 dutZED confirms the latest unique Trust Center Link Key update (Criteria 17-18)
7 Buffer test exchange to verify the new unique key is in place and properly being used (Criteria 19-20)


Pass Verdict:
1 dutZR shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 dutZR is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3 gZC transports current Network key to dutZR using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4 dutZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5 dutZR queries gZC’s stack revision field by sending a node_desc_req and parsing the mating node_desc_rsp.
  If stack revision is less than 21, dutZR skips the Trust Center link key update.
6 dutZR requests Trust Center Link Key from gZC using APS Request-Key (Key Type = 0x04).
7 gZC transports unique Trust Center Link Key using APS Transport-Key (Key Type = 0x04) to dutZR;
  APS Transport-Key encrypted with shared link key.
8 dutZR sends an APS Verify Key Command to gZC with the AES-MMO hash of ‘0x03’ calculated with the unique
  TC link-key. The message is not APS encrypted.
9 gZC sends an APS Verified Key Command to dutZR with status = SUCCESS. The message is APS encrypted with the
  unique TC link-key.
10 dutZED shall issue an MLME Beacon Request MAC command frame, and dutZR shall respond with a beacon.
11 dutZED is able to complete the MAC association sequence with dutZR and gets a new short address, randomly generated.
12 dutZR sends an APS Update-Device command to gZC with APS level encryption using unique Trust Center Link Key,
   with a status code of 0x01 (Standard Device unsecured join).
13 gZC sends an APS Tunnel command to dutZR, containing an APS transport key command that is APS encrypted.
14 dutZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
15 dutZED requests Trust Center Link Key from gZC using APS Request-Key (Key Type = 0x04).
16 gZC transports unique Trust Center Link Key using APS Transport-Key (Key Type = 0x04) to dutZED;
   APS Transport-Key encrypted with shared link key.
17 dutZED sends an APS Verify Key Command to gZC with the AES-MMO hash of ‘0x03’ calculated with the unique
   TC link-key. The message is not APS encrypted.
18 gZC sends an APS Verified Key Command to dutZED with status = SUCCESS. The message is APS encrypted with the
   unique TC link-key.
19 dutZED sends an Buffer test request to gZC; and encrypted at APS level using unique Trust Center Link Key
20 gZC sends Buffer test response to dutZED, encrypted at APS level using unique Trust Center Link Key


Fail Verdict:
1 dutZR does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2 dutZR is not able to complete the MAC association sequence with gZC.
3 gZC does not send an Transport-Key (NWK key) command to dutZR, or gZC does not encrypt the Transport-Key (NWK key)
  at APS level.
4 dutZR does not issue a device announcement.
5 dutZR does not query gZC’s stack revision or continues unique Trust Center link key exchange with a
  legacy TC implementing R20 or below.
6 dutZR does not request Trust Center Link Key from gZC
7 gZC does not transport unique Trust Center Link Key to dutZR; or APS Transport-Key does not encrypted with shared link key
8 dutZR does not send an APS Verify Key Command.
9 gZC does not send an APS Verified Key Command, or command is not encrypted at the APS layer or encryption key
  is not the new unique TC link-key or status is not SUCCESS.
10 dutZED does not issue an MLME Beacon Request MAC command frame, or dutZR does not respond with a beacon.
11 dutZED is not able to complete the MAC association sequence with dutZR.
12 dutZR does not send an APS Update-Device command to gZC, or does not use unique Trust center Link Key for
   APS level encryption, or does not set the status code to 0x01.
13 gZC does not send an APS tunnel command to dutZR.
14 dutZED does not issue a device announcement.
15 dutZED does not request Trust Center Link Key from gZC
16 gZC does not transport unique Trust Center Link Key to dutZED; or APS Transport-Key does not encrypted with shared link key
17 dutZED does not send an APS Verify Key Command.
18 gZC does not send an APS Verified Key Command, or command is not encrypted at the APS layer, or encryption key
   is not the new unique TC link-key, or status != SUCCESS.
19 dutZED does not send Buffer test request to gZC; or Buffer test request is not encrypted with unique
   Trust Center Link Key at APS level
20 gZC does not send Buffer test response to dutZED, or Buffer test response is not encrypted with unique
   Trust Center Link Key at APS level

