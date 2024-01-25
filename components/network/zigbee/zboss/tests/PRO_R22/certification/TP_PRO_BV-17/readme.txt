TP/PRO/BV-17 Address conflict (Remote device)
Objective: To confirm the correct behaviour on detection of an address conflict for a remote device.

NOTE: for this test case, DUT ZED2 can be a ZigBee 2006 device to confirm its correct behaviour.

       DUT ZC
   |            |
DUT ZR1      DUT ZR2
   |            |
 gZED1        gZED2

Initial conditions:
1. Set DUT ZC under target stack profile, DUT ZC as coordinator starts a PAN = 0x1AAA 
network DUT ZR1 shall join the PAN at DUT ZC. DUT ZR2 shall join the PAN at DUT ZC. Via stochastic
addressing, the DUT ZC shall assign logical network addresses to DUT ZR1 and DUT ZR2;

Test procedure:
1. gZED1 shall join the PAN at DUT ZR1;
2. Wait for transmission of Link Status commands to be exchanged between DUT ZR1/DUT ZC and DUT ZR2/DUT ZC;
3. By application specific means make gZED2 join the PAN at DUT ZR2 with DUT ZR2 assigning the same logical network address as gZED1;

Pass verdict:
1. gZED1 shall join the PAN at DUT ZR1 and shall be allocated a logical network address 
in accordance with stochastic address rules;
2. DUT ZR1 shall broadcast a Device-annc command on behalf of gZED1;
3. gZED2 shall join the PAN at DUT ZR2 and shall be allocated a logical network address which is the same as gZED1;
4. DUT ZR2 shall broadcast a Device-annc command on behalf of gZED2;
5. DUT ZR1 shall broadcast a Network Status command with Status Code 0x0d (Address Conflict) 
and Destination Address the same as gZED1 and gZED2;
6. On receipt of the Network Status command, DUT ZR1 and DUT ZR2 shall unicast Rejoin Response 
commands to gZED1 and gZED2 with new (different) logical network addresses.
7. DUT ZR1 and DUT ZR2 shall then broadcast Device-annc with the new logical network addresses of gZED1 and gZED2.

Fail verdict:
1. gZED1 does not join the PAN at DUT ZR1 and is not allocated a
logical network address in accordance with stochastic address rules;
2. DUT ZR1 does not broadcast a Device-annc command on behalf of gZED1;
3. gZED2 does not join the PAN at DUT ZR2 and is not allocated a logical 
network address which is the same as gZED1;
4. DUT ZR2 does not broadcast a Device-annc command on behalf of gZED2;
5. On receipt of the Device-annc command from DUT ZR2, DUT ZC
does not broadcast a Network Status command with Status Code 0x0d (Address Conflict) 
and Destination Address the same as gZED1 and gZED2.
6. On receipt of the Network Status command, DUT ZR1 and DUT ZR2 do not unicast Rejoin Response commands 
to gZED1 and gZED2 (respectively) with new (different) logical network addresses.
7. DUT ZR1 and DUT ZR2 do not broadcast Device-annc with the new
logical network addresses of gZED1 and gZED2 respectively.

Comments:
ieee addr zr1: 01 00 00 00 00 00 00 00 (0x0bc2d)
ieee addr zr2: 02 00 00 00 00 00 00 00
ieee addr ed1: 03 00 00 00 00 00 00 00
ieee addr ed2: 04 00 00 00 00 00 00 00





