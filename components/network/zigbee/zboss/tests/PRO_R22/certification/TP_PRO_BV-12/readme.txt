TP/PRO/BV-12 Address Conflict Resolution by a parent of end device
DUT ZC detects an address conflict with its child and asks child to change its address in conflict resolution

     DUT ZC
   |         |
 gZED1      gZR1

*Need to use Test Profile 3 to send device announcement;
**Per the Test Profile, gZED shall be polling every 2 seconds.


Make sure that link message is broadcastings to neighbouring routers to
communicate their incoming link cost to each other for every 15sec. 
Per the Test Profile, gZED shall be polling every 2 seconds.

Test procedure:
1. gZR1 performs startup procedure and joins the PAN network;
2. gZR1 changes its address internally and sends out an End Dev Annouce with same short address as gZED;

Pass verdict:
1. gZR1 shall issues a MLME_Beacon Request MAC command frame, and DUT ZC shall respond with beacon;
2. Based on the active scan result gZR1 is able to complete the MAC association sequence with DUT ZC;
3. Using TestProfile 3 gZR1 broadcasts a device announcement with short address the same as gZED;
4. DUT ZC shall broadcast a Network Status command with status code addresscode address conflict (0x0d);
5. gZR1 changes its short address;
6. gZR1 broadcasts a device announcement with new short address;
7. DUT ZC shall send an unsolicited Rejoin Response to tell gZED to change its short address;
8. gZED changes its short address;
9. gZED sends a device announcement with a new short address;

Fail verdict:
1. gZR1 does not issue a MLME_Beacon Request MAC command frame, and gZC does not respond with beacon;
2. Based on the active scan result gZR1 is not able to complete the MAC association sequence with gZC;
3. Using TestProfile 3 gZR1 does not broadcast a device announcement with short address the same as DUT ZED;
4. gZC does not broadcast a Network Status command with status code address conflict (0x0d);
5. gZR1 does not change its short address;
6. gZR1 does not broadcast a device announcement with new short address;
7. DUT ZC does not sends a rejoin response to gZED;
8. gZED does not change its short address;
9. gZED does not send a device announcement;










