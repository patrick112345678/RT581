47.4.1.5 Bidirectional commissioning1, ApplicationID = 0b000; OOB key

Initial conditions:
1) TH-GPD uses ApplicationID = 0b000 and GPD SrcID = Z.
2) DUT-GPP and TH-GPS are configured with the same
   gpSharedSecurityKeyType = 0b000 (i.e. no shared key).

Test Procedure:
 - Bidirectional Commissioning
 - After Success GPS reads DUT's Proxy table.
