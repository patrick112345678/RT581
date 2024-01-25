Bidirectional commissioning, ApplicationID = 0b010; OOB key

Initial conditions:
1) TH-GPD uses ApplicationID = 0b010 and GPD IEEE = A and EP = E.
2) DUT-GPP and TH-GPS are configured with the same
   gpSharedSecurityKeyType == 0x000.

Test Procedure:
 - Bidirectional Commissioning
 - After Success GPS reads DUT's Proxy table.
