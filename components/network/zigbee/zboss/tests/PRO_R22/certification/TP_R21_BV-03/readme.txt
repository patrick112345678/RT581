TP/R21/BV-03 Form Distributed Security Network
Objective: DUT ZR forms a distributed security network; DUT ZR short address is not 0x0000; DUT ZR emits MAC
           beacon frames, which have the PAN coordinator flag cleared in their super-frame specification;
           DUT ZR does not respond to MAC frames destined at the PAN coordinator; DUT ZR delivers network
           key to joining gZR; DUT ZR delivers network key to joining gZED with macRXOnWhenIdle = FALSE.

Initial Conditions:

Devices: DUT ZR, gZC1, gZC2, gZC3, gZR, gZED
gZC1: channel #11
gZC2: channel #20
gZC3: channel #25

DUT ZR: channels #11, #15, #20, #25
gZED: RxOffWhenIdle


Test Procedure:
1 Let DUT ZR form a distributed security network (Criteria 1-2)
2 gZR performs an active scan (Criteria 3-5)
3 Optional: If gZR is a test harness or otherwise capable of sending frames destined to the PAN Coordinator at
  the MAC layer (i.e. no MAC destination address in MAC frame header), let it send an arbitrary data frame with
  acknowledgment request flag set in the MHR to the PAN Coordinator. (Criteria 6)
4 Let DUT ZR open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request (Criteria 7)
5 Let gZR join the network formed by DUT ZR (Criteria 8-14)
6 Turn off gZR or make sure it is not permitting joining
7 Let gZED join the network formed by DUT ZR (Criteria 15-17)


Pass Verdict:
1 DUT ZR performs active scans and energy scans on the specified channel mask 0x02108800 (#11, #15, #20, #25).
2 DUT ZR selects channel #15 as the one with lowest number of networks and least background noise and starts operating
  on the network as the first router.
3 gZR sends beacon request frames and DUT ZR responds with beacon frames.
4 The MAC source address in the beacons sent by DUT ZR is in the range 0x0001...0xFFF7
5 The PAN Coordinator flag in the Superframe Specification field of the MAC beacons sent by DUT ZR is clear
6 DUT ZR does not respond to frames destined at the PAN Coordiantor, i.e. it does not respond to or react on frames,
  where the MAC frame header does not contain a destination address
7 DUT ZR responds to beacon requests and the Association Permit flag in the beacon frame is set.
8 DUT ZR accepts MAC Association request from gZR and responds with Association response
  (valid 16-bit network short address, status = SUCCESS), when polled by gZR.
9 DUT ZR sends APS transport key message to gZR within apsSecurityTimeout.
10 The transport key message is unencrypted at the NWK layer.
11 The transport key message is encrypted at the APS layer with the distributed security link key as specified in
   the initial conditions for this test and extended nonce flag set.
12 The transport key message transports the active network key.
13 In the transport key message, the extended source in the payload (Trust Center EUI-64) is equal to
   0xFFFF FFFF FFFF FFFF, indicating no dedicated trust center (distributed security network).
14 DUT ZR emits regular link status messages, with an entry for gZR, where the outgoing cost is non-zero
15 DUT ZR accepts MAC Association request from gZED and responds with Association response
   (valid 16-bit network short address, status = SUCCESS), when polled by gZED.
16 DUT ZR sends APS transport key message to gZED, when polled by gZED within apsSecurityTimeout.
17 Criteria 10, 11, 12, 13 are fulfilled for this transport key message from DUT ZR to gZED.



Fail Verdict:
1 DUT ZR does not emit beacon request frames to scan for existing networks
2 DUT ZR does not emit periodic link status messages on channel #15. Notice: The messages will be encrypted and the key
  might be unknown yet unless the DUT provides an interface to retrieve the NWK key.
3 DUT ZR does not respond with beacon frames
4 The MAC source address in the beacons sent by DUT ZR is in the reserved address range 0xFFF8…0xFFFF or equals 0x0000,
  which is the reserved address for the ZigBee Coordinator and Trust Center
5 The PAN Coordinator flag in the Superframe Specification field of the MAC beacons sent by DUT ZR is set
6 DUT ZR acknowledges or responds to a frame sent to the PAN Coordinator, i.e. it responds to or reacts on frames, where
  the MAC frame header does not contain a destination address
7 DUT ZR does not respond to beacon requests or the Association Permit flag in the beacon frame is clear.
8 DUT ZR does not allow successful completion of MAC association sequence for gZR.
9 DUT ZR does not send APS transport key message to gZR within apsSecurityTimeout.
10 The transport key message uses NWK security.
11 The transport key message is unencrypted at the APS layer or the encryption key used was not the distributed
   security link key or the extended nonce flag was not set.
12 The transport key message does not convey the active network key.
13 In the transport key message, the extended source is different from 0xFFFF FFFF FFFF FFFF.
14 The link status messages emitted by DUT ZR cannot be decrypted with the key transported above, or the link status
    messages don’t list gZR as a neighboring router, or the outgoing cost is zero.
15 DUT ZR does not allow successful completion of MAC association sequence for gZED.
16 DUT ZR does not send APS transport key message to gZED when polled within apsSecurityTimeout or sends transport
   key message without having been polled by gZED.
17 Any of criteria 10, 11, 12, 13 is not fulfilled for this transport key message from DUT ZR to gZED.

