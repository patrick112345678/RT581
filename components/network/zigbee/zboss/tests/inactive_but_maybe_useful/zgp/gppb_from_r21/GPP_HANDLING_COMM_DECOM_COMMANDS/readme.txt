Handling of GPD Commissioning/Decommissioning command in operation
Initial Conditions:
1) TH-GPS, DUT-GPP and TH-GPD
2) TH-GPD, DUT-GPP and TH-GPS use SecurityLevel = 0b10, preferably with
   SecurityKeyType 0b010 (GPD group key), preferably the key being key
   (0:15) = 0xc0 0xc1 0xc2 0xc3 0xc4 0xc5 0xc6 0xc7 0xc8 0xc9 0xca 0xcb 0xcc
            0xcd 0xce 0xcf.

3) TH-GPD uses ApplicationID = 0b000 and GPD SrcID Z.

Test Procedure:
1) The test steps 1A – 1E below can only be performed if the GPD role is taken by a TH
a) GPD Commissioning command in operational mode
b) Negative test: Unprotected GPD Commissioning command in operational mode
c) Negative test: unprotected GPD Commissioning command from unknown GPD in operational mode
d) Negative test: protected GPD Decommissioning command from unknown GPD in operational mode
e) Negative test: Unprotected GPD Decommissioning command in operational mode
2) GPD Decommissioning command in operational mode

Comment: test has questions; current implementation is controversial.
