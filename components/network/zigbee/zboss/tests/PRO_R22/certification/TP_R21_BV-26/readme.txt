TP/R21/BV-26 test for mgmt_lqi_req
Objective: Demonstrate the device’s ability to report its neighbor table to another device on the network.

Initial Conditions:
DUT ZR/ZED/ZC
DUT	Extended Address = any IEEE EUI-64

gZR1	Extended Address = any IEEE EUI-64
gZR2	Extended Address = any IEEE EUI-64
gZR3	Extended Address = any IEEE EUI-64

gZED	Extended Address = any IEEE EUI-64

gZC	Extended Address = any IEEE EUI-64

All devices is visible for each other.


Test Procedure:
1.	Make sure that DUT and gZR1/2/3, gZED and – if DUT is not a ZC - gZC are on the same network and able to communicate, e.g.
•	if DUT is a ZC, join first gZED, then gZR1, gZR2, and gZR3 to DUT ZC; 
•	if DUT is a ZR, let gZC form a centralized network and let DUT ZR, gZR1, gZR2, gZR3 join; the let gZED join at DUT ZR;
•	if DUT is a ZED, let gZC form a centralized network and let DUT ZED join; then let gZR1, gZR2, gZR3 join.
2.	Wait for at least 45 seconds
3.	Let any of gZR1, gZR2, gZR3 or gZC send as many mgmt._lqi_req, with properly incrementing start index as required to retrieve DUT’s complete neighbor table

Pass Verdict:
1.	DUT responds with a number of mgmt._lqi_rsp frames, which reveal its neighbor table.
2.	If DUT is a ZED:
The NeighborTableEntries field is 1 and the only entry is that of the parent device (gZC), with type set to ‘Coordinator’ and relation set to ‘parent’, RX-on-when-idle = ‘receiver on’ or ‘unknown’, the Extended PAN ID matching that of the network, the Extended Address matching that of gZC, the short address matching that of gZC. 
3.	If DUT is a ZR:
The NeighborTableEntries field is 5 in each response frame returned. The contents of each frame reflect the subset selected by start index of the according request frame. All routers and the coordinator have a relationship of ‘sibling’ or ‘child’; one might have a relation of ‘parent’. The end device has a relation of ‘child’. All short and extended addresses match their expected values. Device types either match or are set to unknown.
4.	If DUT is a ZC:
The NeighborTableEntries field is 5 in each response frame returned. The contents of each frame reflect the subset selected by start index of the according request frame. All routers have a relationship of ‘sibling’ or ‘child’; one might have a relation of ‘parent’. The end device has a relation of ‘child’. All short and extended addresses match their expected values. Device types either match or are set to unknown.

Fail Verdict:
1.	DUT does not respond with mgmt._lqi_rsp frames, or the status is NOT_SUPPORTED.
2.	There is more than one entry, or no entry, or the relation is not ‘parent’, or the type is not ‘Coordinator’, or RX-on-when-idle is ‘Receiver off’, or the Extended PAN ID is incorrect, or the Extended Address is incorrect, or the short address is incorrect.
3.	The total number of neighbors does not equal 5. Not all 5 entries are returned after the required number of mgmt._lqi_req frames with appropriate start index have been issued. The relationship is set to ‘none’ for any of the devices on the network, the extended PAN-ID does not match, or any of the short and extended addresses do not match.
4.	The total number of neighbors does not equal 5. Not all 5 entries are returned after the required number of mgmt._lqi_req frames with appropriate start index have been issued. The relationship is set to ‘none’ for any of the devices on the network, the extended PAN-ID does not match, or any of the short and extended addresses do not match.


Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.

tp_r21_bv-26_dutzc.c
tp_r21_bv-26_gzed.c
tp_r21_bv-26_gzr1.c
tp_r21_bv-26_gzr2.c
tp_r21_bv-26_gzr3.c


tp_r21_bv-26_gzc.c
tp_r21_bv-26_dutzr.c
tp_r21_bv-26_gzed.c
tp_r21_bv-26_gzr1.c
tp_r21_bv-26_gzr2.c
tp_r21_bv-26_gzr3.c


tp_r21_bv-26_gzc.c
tp_r21_bv-26_dutzed.c
tp_r21_bv-26_gzr1.c
tp_r21_bv-26_gzr2.c
tp_r21_bv-26_gzr3.c
