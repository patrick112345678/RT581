TP/PRO/BV-05 Symmetric Links - Link Status Command
Objective: To confirm the correct transmission of Link Status commands within a nwkSymLink = TRUE network.

        gZC
   |          |
  DUT ZR1     gZR2


gZC	PANid= 0x1AAA
	Logical Address = 0x0000
	0x aa aa aa aa aa aa aa aa
	PANid=0x1AAA

DUT ZR1	PANid=0x1AAA
        Logical Address=Generated in a random manner (within the range 1 to 0xFFF7)
        IEEE address=0x00 00 00 01 00 00 00 00

gZR2	PANid=0x1AAA
        Logical Address=Generated in a random manner (within the range 1 to 0xFFF7)
        IEEE address=0x00 00 00 02 00 00 00 00

Initial consitions:
1) Reset all nodes
2) Set gZC under target stack profile. gZC as coordinator starts a PAN = 0x1AAA network and is the
   Trust Centre for the PAN.
3) DUT ZR1 shall join PAN at gZC, gZR2 shall join PAN at gZC
Where gZC and gZR2 are golden units


Test procedure:
1) Wait 1 minute (or 4 x nwkLinkStatusPeriod if not using default values)


Pass verdict:
1) gZC, DUT ZR1 and gZR2 shall broadcast (1 hop only) Link Status commands every nwkLinkStatusPeriod (± random jitter)

Fail verdict:
1) gZC, DUT ZR1 and gZR2 do not broadcast (1 hop only) Link Status commands every
   nwkLinkStatusPeriod (± random jitter)
