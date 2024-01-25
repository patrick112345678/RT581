TP/R21/BV-10: Trust Center Link-Key Update - for ZR/ZED, legacy TC
Objective: DUT ZR/ZED joins centralized security network fomed by gZC (R20) and gZR; gZC is not within radio range of
           DUT ZR, i.e. DUT ZR has no knowledge of gZC’s short address; DUT configured with “requestLinkKey = TRUE”;
           DUT joins via association, requests unique TC link-key; gZC may or may not respond with unique TC link key;
           DUT sends node_desc_req ZDO message to gZC not using APS encryption; upon receipt of a generic ZDO NOT_SUPPORTED
           response status, the node remains on the network and assumes it has authenticated successfully; in case of
           timeout or a status code other that NOT_SUPPORTED, the node leaves and attempts to join either another open
           network or the same network.

Initial conditions:

gZC - DUR ZR - DUT ZED

gZC:
LEGACY GU, running ZigBee PRO stack compliant with revision 20 of the ZigBee core specification
Extended Address = any IEEE EUI-64

dutZR:
Extended Address = any IEEE EUI-64

dutZED:
Extended Address = any IEEE EUI-64



Test procedure:
1 Join device dutZR to legacy gZC. (Criteria 1-4)
2 dutZR determines Trust Center as legacy (R20 or earlier) and refrains from requesting a unique link key from
  the Trust Center (Criteria 5-6)
3 dutZR remains on the network (Criteria 7)
4 Join device dutZED to dutZR. (Criteria 8-12)
5 dutZED determines Trust Center as legacy (R20 or earlier) and refrains from requesting a unique link key from
  the Trust Center (Criteria 13-14)
6 Wait for two minutes (Criteria 15)
7 dutZED remains on the network (Criteria 15-17)


Pass Verdict:
1 dutZR shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 dutZR is able to discover the network formed by gZC and complete the MAC association sequence with gZC and is
  assigned a new short address, randomly generated.
3 gZC transports current Network key to dutZR using APS Transport-Key; APS Transport-Key encrypted with shared
  link key at APS level.
4 dutZR issues a ZDO device announcement sent to the broadcast address (0xFFFD) encypte with the active NWK key.
5 dutZR queries gZC’s stack revision field by sending a node_desc_req and parsing the mating node_desc_rsp.
  It detects a stack revision less than 21, dutZR skips the Trust Center link key update.
6 dutZR does not request a Trust Center Link Key from gZC.
7 dutZR emits regular link status messages with an entry for gZC, where incoming and outgoing cost are non-zero.
  At least 8 link status messages shall be observed over a period of approximately two minutes.
8 dutZED shall issue an MLME Beacon Request MAC command frame, and dutZR shall respond with a beacon.
9 dutZED is able to discover the network formed by gZC and complete the MAC association sequence with dutZR and is
  assigned a new short address, randomly generated.
10 dutZR sends an APS Update-Device command to gZC with and without APS level encryption using shared/gobal Trust
   Center Link Key, with a status code of 0x01 (Standard Device unsecured join).
11 dutZR forwards the transport key message enveloped in the APS tunnel command sent by gZC to dutZR to gZED.
12 dutZED issues a ZDO device announcement sent to the broadcast address (0xFFFD) using NWK encryption.
13 dutZED queries gZC’s stack revision field by sending a node_desc_req and parsing the mating node_desc_rsp.
   It detects a stack revision less than 21, dutZED skips the Trust Center link key update.
14 dutZED does not request a Trust Center Link Key from gZC.
15 dutZED remains on the network
16 dutZED sends an Buffer test request to gZC; encrypted at APS level using shared/global Trust Center Link Key
17 gZC sends Buffer test response to dutZED, encrypted at APS level using shared Trust Center Link Key



Fail Verdict:
1 dutZR does not issue an MLME Beacon Request MAC command frame.
2 dutZR fails to discover the network, or does not request MAC association or does not poll for the association
  response in due time or otherwise failes to complete the MAC association sequence with gZC.
3 This operation cannot fail for dutZR. It fails for gZC if it does not send the Transport-Key (NWK key)
  command in due time or does not APS encrypt with the global link-key.
4 dutZR does not issue a device announcement or does not apply NWK level encryption with the correct key.
5 dutZR does not query gZC’s stack revision or continues unique Trust Center link key exchange although gZC returns
  a server mask with the stack compliance revision field set to 0 or the response status indicating NOT_SUPPORTED.
6 dutZR does request a Trust Center Link Key from gZC.
7 dutZR does not emit regular link status messages, or link status does not have an entry for gZC, or the
  incoming or outgoing cost from/to gZC is zero, or dutZR broadcasts leave indication or performs active scans.
8 dutZED does not issue an MLME Beacon Request MAC command frame, or dutZR does not respond with a beacon.
9 dutZED fails to discover the network, or does not request MAC association or does not poll for the association
  response in due time or dutZR does not respond with MAC association response when polled by dutZED or MAC association sequence fails otherwise.
10 dutZR does not send an APS Update-Device command to gZC, or does not use shared Trust center Link Key for APS
   level encryption, or does not send a second copy of Update Device without APS encryption or does not set the
   status code to 0x01.
11 dutZR does not forward the APS transport key command to dutZED.
12 dutZED does not issue a device announcement or device_annce is not encrypted with the active network key.
13 dutZED does not query gZC’s stack revision or continues unique Trust Center link key exchange although gZC
   returns a server mask with the stack compliance revision field set to 0 or the response status indicating NOT_SUPPORTED.
14 dutZED does request a Trust Center Link Key from gZC.
15 dutZED broadcasts leave indication or performs active scans.
16 dutZED does not send Buffer test request to gZC; or Buffer test request is not encrypted with shared/global
   Trust Center Link Key at APS level
17 gZC does not send Buffer test response to dutZED, or Buffer test response is not encrypted with shared Trust Center
   Link Key at APS level

