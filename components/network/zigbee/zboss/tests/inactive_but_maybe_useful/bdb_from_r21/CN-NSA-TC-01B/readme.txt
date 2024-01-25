4.5.2 CN-NSA-TC-01B: Mgmt_Permit_Joining_req sending on distributed network
This test verifies the behavior of a DUT ZR after sending Mgmt_Permit_Joining_req upon successfully joining a distributed network. 
The DUT shall take the role defined in its PICS; the other role can be performed by a golden unit or test harness, as indicated.

Required devices:
DUT - ZR

Additional devices:
THe1 - TH ZED (this role can be performed by a TH)
THr1 - TH ZR 


Preparatory steps:
P1 - THr1 is triggered to establish a distributed network on one of the primary channels supported by the DUT.
P2 - Network is closed (AssociationPermit of THr1 is set to FALSE, e.g. because it expires).


Test Procedure:
 - Trigger network steering on the THc1
 - Trigger network steering on the DUT
 - Wait for DUT joins
 - Trigger network steering on the THe1


