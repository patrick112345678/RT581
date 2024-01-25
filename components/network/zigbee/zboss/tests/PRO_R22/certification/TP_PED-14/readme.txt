TP/PED-14 Child Table Management: Parent Announcement – Splitting & Randomization
Objective: Demonstrate ability to spread a large number of child entries over multiple parent_annce frames (splitting). 

Initial Conditions:

The required network setup for the test is depicted below. Notice that it is possible to run the test with a
single Golden Unit End-Device, if that device is capable of changing its IEEE address. In this case make sure
25 distinct addresses are assigned to the gZED.

DUT - ZR/ZC

  DUT - 25 gZED

Test Procedure:
1.	Make sure that DUT and all gZEDs (25 in total) are on the same network and able to communicate, e.g.
•	if DUT is a ZC, join gZR to DUT ZC; 
•	if DUT is a ZR, let gZR form a distributed security network and let DUT ZR join
Notice: If a single GU is used instead, the procedure is to assign the first of 25 unique IEEEs to the gZED, then join it to the DUT. Then factory reset gZED and make sure it doesn’t send a leave indication; then assign another IEEE address out of the pool of 25 unique addresses and join the device again; repeat the process another 25 times… 
2.	Power-cycle the DUT 
3.	Observe DUT’s over-the-air behavior

Pass Verdict:
1.	After the DUT is turned on, it broadcasts a first parent_annce with 10 child info records no earlier than 10 seconds after restart and no later than 20 seconds after restart. 
2.	No earlier than 10 seconds after the first parent_annce, and no later than 20 seconds after the first parent_annce, the DUT broadcasts another parent_annce with another 10 child record enties not seen before.
3.	No earlier than 10 seconds after the second parent_annce, and no later than 20 seconds after the second parent_annce, the DUT broadcasts another (the third) parent_annce with the remaining 5 child record entries not seen before.
4.	All 25 unique EUI-64 have been announced in the three parent_annce messages.
5.	The exact timing within the 10...20 second interval shows random variance for each of the three instances of parent_annce.

Fail Verdict:
1.	DUT does not broadcast parent_annce; or parent_annce contains less than the first 10 entries; or the broadcast appears sooner than 10 seconds after reboot, or later than 20 seconds after reboot. A tolerance of one second is permissible.
2.	DUT does not broadcast parent_annce; or parent_annce contains less than the second 10 entries; or the broadcast appears sooner than 10 seconds after the initial parent_ancce, or later than 20 seconds after the initial parent_annce. A tolerance of one second is permissible.
3.	DUT does not broadcast parent_annce; or parent_annce contains less than the remaining 5 entries; or the broadcast appears sooner than 10 seconds after the second parent_ancce, or later than 20 seconds after the second parent_annce. A tolerance of one second is permissible.
4.	Any of the 25 unique EUI-64 was not announced.
5.	The timing within the 10...20 second interval appears to be identical for all the three instances of parent_annce.

Notes:
 - If the fifth criterium is not met, it is permissible to power-cycle the DUT and check the randomness of
   the parent_annce messages without executing the whole test from scratch.


Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc or zr: i.e. ./runng zc to start test with DUT as ZC.
 - It is possible to run this test with 1 gzed capable of changing it's IEEE address.
   To make this go to runng.sh, comment line #97 and uncomment line #155.
