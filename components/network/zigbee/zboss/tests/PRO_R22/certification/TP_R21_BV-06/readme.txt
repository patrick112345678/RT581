12.6 TP/R21/BV-06 Timeout of joiner
Objective: Demonstrate the ability that a ZR/ZED with nwkSecurityLevel > 0 that does not receive a suitable transport
key message with the current NWK key within apsSecurityTimeout after successful association shall leave the network and
start-over.


Test setup:

  gZC
   |
   |   
  gZR
   |
   |
  DUT

DUT can be ZR or ZED.

Test Procedure:
1 Let gZC form a centralized network
2 Let gZC open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request
3 Let gZR join the network formed by gZC
4 Let gZR open the network for new devices to join via mgmt_permit_join_req or NLME-PERMIT-JOIN.request
5 Turn off gZC
6 Let DUT join the network at gZR

Pass Verdict:
1 DUT performs active scans and sends beacon request frames. It discovers the network formed by gZC and successfully performs MAC association at gZR.
DUT does not emit beacon request frames or fails to discover the network formed by gZC, or it does not send MAC association request, or it does not poll for MAC association response at due time.
2 DUT does not receive a transport message within apsSecurityTimeout (because gZC is offline) and leaves the network.
DUT remains on the network without a NWK key.
3 DUT ceases NWK operation and resets the stack. It does not attempt to send a leave indication.
DUT sends a leave indication, unencrypted
4 DUT may start-over, i.e. it may perform an active scan under application control.

Fail verdict:
1 DUT does not emit beacon request frames or fails to discover the network formed by gZC, or it does not send MAC association request, or it does not poll for MAC association response at due time.
2 DUT remains on the network without a NWK key.
3 DUT sends a leave indication, unencrypted
4 None (optional behaviour for core stack)
