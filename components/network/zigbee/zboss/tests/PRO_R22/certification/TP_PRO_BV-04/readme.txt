TP/PRO/BV-04 Asymmetric Link - Replacement
Objective: To confirm the correct replacement of asymmetric links with symmetric links within Neighbor/Routing Tables
NOTE: Ageing process depends on NIB attributes:
    nwkLinkStatusPeriod = 0x00 to 0xff seconds (0x0f; 15s default)
    nwkRouterAgeLimit = 0x00 to 0xff (0x03; 3 default)

        gZC
   |           |
DUT ZR1     DUT ZR2
               |
             gZED1

gZC	PANid= 0x1AAA
	Logical Address = 0x0000
	0x aa aa aa aa aa aa aa aa
	PANid=0x1AAA

DUT ZR1	Logical Address=Generated in a random manner (within the range 1 to 0xFFF7)
        IEEE address=0x00 00 00 01 00 00 00 00
	    
gZED1   RxOnWhenIdle=TRUE
	    IEEE address=0x00 00 00 02 00 00 00 00

DUT ZR2	PANiD=0x1aaa
	    IEEE address=0x00 00 00 03 00 00 00 00
	    lower power transmission

Initial conditions:
1. Reset all nodes; SYMLINK=TRUE
2. Set gZC under target stack profile. gZC as coordinator starts a PAN = 0x1AAA network and is the Trust Center for the PAN.
3. DUT ZR2 shall join PAN at gZC, DUT ZR1 shall join PAN at gZC, gZED1 joins PAN at DUT ZR2.

Test procedure:
Via test driver gZC shall unicast Buffer Test Request to gZED1

Pass verdict:
1) gZC sends Buffer test request for gZED1.
2) gZC shall broadcast a route request for gZED1
3) Both DUT ZR1 and DUT ZR2 reply to the route request
4) gZED1 sends Buffer test response along the path gZED1 -> DUT ZR2 -> DUT ZR1 -> gZC

Fail verdict:
1)	The buffer test request is not routed along the path gZC -> DUT ZR1 -> DUT ZR2 -> gZED1
2)	The buffer test response is not routed along the path gZED1 -> DUT ZR2 -> DUT ZR1 -> gZC

