TP/PRO/BV-13 Address Conflict Resolution for an end device by parent
Verify on receipt of address conflict network message, the parent of an end device
changes its child short address.

     DUT ZC
   |         |
gZR1         gZED1


At HW:
- run zc
- run zed
- run zr


*Using Test Profile 3 (currently not used)
** Test Profile 3 only needed in tests: TP/PRO/BV-42 43, TP/PRO/BV-07 13, TP/PRO/BV-06 12;

Test procedure 
1. gZR1 shall broadcast a Network Status with status code 
Address Conflict (0x0d) indicating short address of gZED;
2. gZR1 shall unicast a Link Status with short address same as gZED;

Pass verdict:
1. gZR1 shall broadcast a Network Status with status code Address Conflict (0x0d) indicating short address of gZED;
2. DUT ZC shall unicast an unsolicited Rejoin Response command with new short address for gZED;
3. DUT ZC broadcasts a device announcement on behalf of gZED;
4. gZR1 shall unicast a Link Status with short address same as gZED;
5. DUT ZC shall unicast an unsolicited Rejoin Response command with new short address for gZED;
6. DUT ZC broadcasts a device announcement on behalf of gZED;

Fail verdict:
1. gZR1 does not broadcast a Network Status with status code Address Conflict (0x0d) indicating short address of gZED;
2. DUT ZC does not unicast an unsolicited Rejoin Response command with new short address for gZED;
3. DUT ZC does not broadcast a device announcement on behalf of gZED;
4. gZR1 does not unicast a Link Status with short address same as gZED;
5. DUT ZC does not unicast an unsolicited Rejoin Response command with new short address for gZED;
6. DUT ZC does not broadcast a device announcement on behalf of gZED;
