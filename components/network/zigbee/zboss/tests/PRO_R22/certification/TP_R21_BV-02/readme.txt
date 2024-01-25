12.2 TP/R21/BV-02 Update Device upon Network Leave
Objective: DUT ZR notifies gZC of gZR or gZED leaving the network


       gZC
    |       |
  DUT ZR - gZR
    |
  gZED
  
DUT ZR	
Extended Address = any IEEE EUI-64
gZR	Extended Address = any IEEE EUI-64

gZED	
Extended Address = any IEEE EUI-64

gZC	
Extended Address = any IEEE EUI-64

Initial Conditions:
1. Reset all nodes
2. DUT ZR
nwkExtendedPANID = 0x0000 0000 0000 0000
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0000
apsUseInsecureJoin = TRUE
apsTrustCenterAddress = 0x0000 0000 0000 0000
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: empty
nwkActiveKeySeqNumber: 0x00
The device holds multiple TC link-keys including the default TC link-key (“ZigBeeAlliance09”), the distributed security link-key for uncertified devices D0:D1:D2:D3:D4:D5:D6:D7:D8:D9:DA:DB:DC:DD:DE:DF and a pre-configured link-key. How these keys are stored is implementation-specific.
3. gZR
nwkExtendedPANID = 0x0000 0000 0000 0000
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0000
apsUseInsecureJoin = TRUE
apsTrustCenterAddress = 0x0000 0000 0000 0000
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: empty
nwkActiveKeySeqNumber: 0x00
The device holds multiple TC link-keys including the default TC link-key (“ZigBeeAlliance09”), the distributed security link-key for uncertified devices D0:D1:D2:D3:D4:D5:D6:D7:D8:D9:DA:DB:DC:DD:DE:DF and a pre-configured link-key. How these keys are stored is implementation-specific.
4. gZED
nwkExtendedPANID = 0x0000 0000 0000 0000
apsDesignatedCoordinator = FALSE
apsUseExtendedPANID = 0x0000 0000 0000 0000
apsUseInsecureJoin = TRUE
apsTrustCenterAddress = 0x0000 0000 0000 0000
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: empty
nwkActiveKeySeqNumber: 0x00
The device holds multiple TC link-keys including the default TC link-key (“ZigBeeAlliance09”), the distributed security link-key for uncertified devices D0:D1:D2:D3:D4:D5:D6:D7:D8:D9:DA:DB:DC:DD:DE:DF and a pre-configured link-key. How these keys are stored is implementation-specific.
5. gZC
nwkExtendedPANID = 0x0000 0000 0000 0000
apsDesignatedCoordinator = TRUE
apsUseExtendedPANID = 0x0000 0000 0000 0000
apsUseInsecureJoin = TRUE
apsTrustCenterAddress = 0x0000 0000 0000 0000
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: randomly generated
nwkActiveKeySeqNumber: 0x00
The device holds multiple TC link-keys including the default TC link-key (“ZigBeeAlliance09”), the distributed security link-key for uncertified devices D0:D1:D2:D3:D4:D5:D6:D7:D8:D9:DA:DB:DC:DD:DE:DF and a pre-configured link-key. How these keys are stored is implementation-specific.

Test Procedure:
1. Let gZC form a centralized network
2. Let gZR join at gZC
3. Let DUT ZR join at gZC or gZR
4. Let gZED join at DUT ZR
5. Let gZED leave and rejoin the network by sending a mgmt_leave_req from gZC to gZED with rejoin = 1
6. Let gZED leave the network by sending a mgmt_leave_req from gZC to gZED with rejoin = 0
7. Let gZR leave and rejoin the network by sending a mgmt_leave_req from gZC to gZR with rejoin = 1
8. Let gZR leave the network by sending a mgmt_leave_req from gZC to gZR with rejoin = 0

Pass verdict:
1. DUT ZR successfully disovers the network formed by gZC, associates, receives the network key, boradcasts device_annce with network security and emits regular link status messages with entries for gZC, gZR. 
2. DUT ZR allows gZED to associate via MAC association, sends update device to gZC (insecure join), and tunnels the transport key message when polled by gZED.
3. DUT ZR receives NWK leave indication from gZED, with rejoin = 1 and does not send update device to gZC with status ‘device left’. When it receives the NWK rejoin request, it notifies the TC by sending an update device with status ‘secure rejoin’ to the TC.
4. DUT ZR receives NWK leave indication from gZED, with rejoin = 0 and sends update device to gZC (‘device left’, gZED).
5. This pass/fail criteria is optional.  If this step fails it shall not trigger a failure of the DUT. DUT ZR receives NWK leave indication from gZR, with rejoin = 1 and does not send update device to gZC. When it receives the NWK rejoin request, it notifies the TC by sending an update device with status ‘secure rejoin’ to the TC.
6. This pass/fail criteria is optional.  If this step fails it shall not trigger a failure of the DUT.DUT ZR receives NWK leave indication from gZR, with rejoin = 0 and sends update device to gZC (‘device left’, gZR).

Fail Verdict
1. DUT ZR does not discover the network formed by gZC, fails to associate, does not broadcast deivce_annce encrypted at the network layer, or does not emit regular link status messages with entries for gZC, gZR.
2. DUT ZR does not allow successful joining of gZED.
3. DUT ZR sends update device (‘device left’, gZED) to gZC; or it does not send update device with status ‘secure rejoin’ to the TC.
4. DUT ZR does not send update device (‘device left’, gZED) to gZC.
5. DUT ZR sends update device (‘device left’, gZR) to gZC; or it does not send update device with status ‘secure rejoin’ to the TC.
6. DUT ZR does not send update device (‘device left’, gZR) to gZC.



     
