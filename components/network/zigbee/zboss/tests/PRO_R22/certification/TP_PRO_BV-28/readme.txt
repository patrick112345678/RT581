10.28 TP/PRO/BV-28 Network Formation Address Assignment
Validation of Random number generation.

DUT ZC	PANid= Generated in a random manner(within the range 0x1 to 0x3FFF)
	Logical Address = 0x0000
	0xaa aa aa aa aa aa aa aa 
gZED1	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 01
gZED2	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 02
gZED3	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 03
gZED4	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 04
gZED5	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 05
gZED6	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 06
gZED7
	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 07
gZED8	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 08
gZED9	Pan id=same PANID  of  ZC	
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 09
gZED10
	Pan id=same PANID  of  ZC
	Logical Address=Generated in a random manner(within the range 1 to 0xFFF7)
	IEEE address=0x00 00 00 00 00 00 00 0A

Initial conditions:
1. Reset ZC
2. Set gZC, under target stack profile,gZC as co-coordinator starts a PAN with a random value (within the range 0x1 to 0x3FFF)

Test procedure:
1 gZED1 performs startup procedure and joins the PAN network with Rx on idle = True
2 gZED2 performs startup procedure and joins the PAN network with Rx on idle = True
3 gZED3 performs startup procedure and joins the PAN network with Rx on idle = True
4 gZED4 performs startup procedure and joins the PAN network with Rx on idle = True
5 gZED5 performs startup procedure and joins the PAN network with Rx on idle = True
6 gZED6 performs startup procedure and joins the PAN network with Rx on idle = True
7 gZED7 performs startup procedure and joins the PAN network with Rx on idle = True
8 gZED8 performs startup procedure and joins the PAN network with Rx on idle = True
9 gZED9 performs startup procedure and joins the PAN network with Rx on idle = True
10 gZED10 performs startup procedure and joins the PAN network with Rx on idle = True
11 By application specific means, reset all addresses and table entries within DUT ZC, gZED1, gZED2, gZED3, gZED4, gZED5, gZED6, gZED7, gZED8, gZED9, and gZED10.
12 Repeat items 1 – 12 a further 9 times.

Pass verdict:
1) On startup each gZED shall  issue an MLME_Beacon Request MAC command frame, and DUT ZC shall respond with beacon. 
Based on the active scan result the gZED shall complete the MAC association sequence with DUT ZC 
and get a new (unrepeated) short address from the DUT ZC (Generated in a random manner within the range 1 to 0xFFF7)
The gZED shall then issue a device Announce
2) A total of 100 unrepeated short addresses shall be assigned by the DUT ZC (ie. 10 gZEDs x 10 startups) 

Fail verdict:
1) On startup each gZED does not issue an MLME_Beacon Request MAC command frame, and DUT ZC does not respond with a beacon. 
Based on the active scan result the gZED does not complete the MAC association sequence with DUT ZC and does not get 
a new (unrepeated) short address from the DUT ZC (Generated in a random manner within the range 1 to 0xFFF7)
The gZED does not then issue a device Announce
2) A total of 100 unrepeated short addresses are not assigned by the DUT ZC (ie. 10 gZEDs x 10 startups)

Comments:


