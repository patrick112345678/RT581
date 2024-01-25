12.4	TP/R21/BV-04 Join Distributed Security Network as Router
Objective:  DUT ZR joins a distributed security network formed by gZR1; DUR ZR allows further devices gZR2, gZED onto the network and delivers the network key.

DUT ZR	Extended Address = any IEEE EUI-64
gZR1	Extended Address = any IEEE EUI-64
gZR2	Extended Address = any IEEE EUI-64
gZED	Extended Address = any IEEE EUI-64

Test procedure:
1. Let gZR1 form a distribute security network
2. Let gZR1 open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request
3. Let DUT ZR join the network formed by gZR1
4. Turn off gZR1
5. Let DUT ZR open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request
6. Let gZR2 join the network at DUT ZR
7. Turn off gZR2 or make sure it is not permitting joining
8. Let gZED join the network at DUT ZR

Pass verdict:
1. DUT ZR performs active scans and sends beacon request frames. It discovers the distributed security network formed by gZR1 and successfully performs MAC association.
2. DUT ZR is able to decrypt the transport key message using its distributed security link key and broadcasts device_annce, encrypted with the active network key.
3. DUT ZR emits regular link status messages, with an entry for gZR1, where the outgoing cost is non-zero.
4. DUT ZR responds to beacon requests and the Association Permit flag in the beacon frame is set.
5. DUT ZR accepts MAC Association request from gZR2 and responds with Association response (valid 16-bit network short address, status = SUCCESS), when polled by gZR2.
6. DUT ZR sends APS transport key message to gZR2 within apsSecurityTimeout.
6a. DUT ZR does not attempt to send an update device message
7. The transport key message is unencrypted at the NWK layer.
8. The transport key message is encrypted at the APS layer with the distributed security link key as specified in the initial conditions for this test and extended nonce flag set.
9. The transport key message transports the active network key.
10. In the transport key message, the extended source in the payload (Trust Center EUI-64) is equal to 0xFFFF FFFF FFFF FFFF, indicating no dedicated trust center (distributed security network).
11. DUT ZR emits regular link status messages, with an entry for gZR2, where the outgoing cost is non-zero
12. DUT ZR accepts MAC Association request from gZED and responds with Association response (valid 16-bit network short address, status = SUCCESS), when polled by gZED.
13. DUT ZR sends APS transport key message to gZED, when polled by gZED within apsSecurityTimeout.
13a. DUT ZR does not attempt to send an update device message
14. Criteria 7, 8, 9, 10 are fulfilled for this transport key message from DUT ZR to gZED.

Fail verdict:
1. DUT ZR does not emit beacon request frames or fails to discover the distributed security network formed by gZR, or it does not send MAC association request, or it does not poll for MAC association response at due time.
2. DUT ZR is unable to decrypt the transport key message sent by gZR1 or does not broadcast device_annce or device_annce is not properly secured at the NWK layer.
3. The link status messages emitted by DUT ZR cannot be decrypted with the key transported above, or the link status messages don’t list gZR1 as a neighboring router, or the outgoing cost is zero. 
4. DUT ZR does not respond to beacon requests or the Association Permit flag in the beacon frame is clear.
5. DUT ZR does not allow successful completion of MAC association sequence for gZR2.
6. DUT ZR does not send APS transport key message to gZR2 within apsSecurityTimeout.
6a. DUT ZR emits an update device message
7. The transport key message uses NWK security.
8. The transport key message is unencrypted at the APS layer or the encryption key used was not the distributed security link key or the extended nonce flag was not set.
9. The transport key message does not convey the active network key.
10. In the transport key message, the extended source is different from 0xFFFF FFFF FFFF FFFF.
11. The link status messages emitted by DUT ZR cannot be decrypted with the key transported above, or the link status messages don’t list gZR2 as a neighboring router, or the outgoing cost is zero. 
12. DUT ZR does not allow successful completion of MAC association sequence for gZED.
13. DUT ZR does not send APS transport key message to gZED when polled within apsSecurityTimeout or sends transport key message without having been polled by gZED.
13a. DUT ZR emits an update device message
14. Any of criteria 7, 8, 9, 10 is not fulfilled for this transport key message from DUT ZR to gZED.

