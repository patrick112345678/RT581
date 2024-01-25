TP/R21/BV-05 Join Distributed Security Network as Sleeping End-Devcice
Objective: DUT ZED joins a distributed security network formed by gZR


Initial conditions:

   gZR
    |
    |
 DUT ZED



Test Procedure:
1 Let gZR form a distribute security network
2 Let gZR open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request
3 Let DUT ZED join the network formed by gZR (Criteria 1-3)


Pass Verdict:
1 DUT ZED performs active scans and sends beacon request frames. It discovers the distributed security
  network formed by gZR and successfully performs MAC association.
2 DUT ZED polls gZR for a transport key message one or more times within apsSecurityTimeout
3 DUT ZED is able to decrypt the transport key message using its distributed security link key and broadcasts
  device_annce, encrypted with the active network key.


Fail Verdict:
1 DUT ZED does not emit beacon request frames or fails to discover the distributed security network formed by gZR,
  or it does not send MAC association request, or it does not poll for MAC association response at due time.
2 DUT ZED does not poll after association or stops polling before apsSecurityTimeout
3 DUT ZED is unable to decrypt the transport key message sent by gZR when polled or does not broadcast
  device_annce or device_annce is not properly secured at the NWK layer.