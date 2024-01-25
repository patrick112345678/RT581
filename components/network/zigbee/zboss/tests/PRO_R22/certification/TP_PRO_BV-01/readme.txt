TP/PRO/BV-01 Router – Joins the network
Verify that the router device is capable to join a network.

gZC
DUT ZR

Test procedure:
1. DUT ZR performs startup procedure and joins the PAN network

Pass verdict:
1) DUT ZR shall issue a MLME_Beacon Request MAC commend frame, and gZC shall respond with beacon.
2) Based on the active scan result DUT ZR is able to complete the MAC association sequence with gZC and gets new short 
address from Coordinator (Generated in a random manner within the range 1 to 0xFFF7)
3) DUT ZR makes a device announcement 
4) A route discovery request broadcast packet may be sent by DUT ZR to find path to gZC. If a route discovery request broadcast packet is sent then gZC shall reply to route request frame with a route response packet to DUT ZR.

Fail verdict:
1) DUT ZR does not issue a MLME_Beacon Request MAC commend frame, and gZC does not respond with beacon.
2) Based on the active scan result DUT ZR not able to complete the MAC association sequence with gZC and does not get new short address.
3) DUT ZR does not make a device announcement.
4) If DUT ZR sends a route request, gZC does not reply to route request frame with a route response packet to DUT ZR.



Comments:
Test procedure: has 2 devices - ZC and ZR
ZC does Formation procedure, including Energy and Active scan.
ZR does Discovery then Join to the network at ZC.
Security is switched off.
ZBOSS DUT ZR never sends a route request in such conditions.

To run this test, type:
sh run.sh

After test complete analyze traffic dump files.
