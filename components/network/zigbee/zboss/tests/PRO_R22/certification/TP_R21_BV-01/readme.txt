TP/R21/BV-01 ZDO message Node Descriptor Stack Revision
Objective: DUT ZC/ZR/ZED responds to ZDO node_desc_req with a node_desc_rsp where status = SUCCESS,
stack compliance revision field = 21. Stack compliance is part of the server mask bitmap.

Initial Conditions:

    DUT
     |
     |
    gZR

1. Reset all nodes
2. DUT ZR/ZED/ZC
nwkExtendedPANID = 0x0000 0000 0000 0000
apsUseExtendedPANID = 0x0000 0000 0000 0000
apsUseInsecureJoin = TRUE
apsTrustCenterAddress = 0x0000 0000 0000 0000
nwkSecurityLevel: 0x05
nwkSecurityMaterialSet.key: empty/randomly generated
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


Test Procedure:
1 Make sure that DUT and gZR (test harness) are on the same network and able to communicate, e.g.
   - if DUT is a ZC, join gZR to DUT ZC;
   - if DUT is a ZR, let gZR form a distributed security network and let DUT ZR join;
   - if DUT is a ZED, let gZR form a distributed security network and let DUR ZED join.
2 Let gZR send a ZDO node_desc_req to DUT

Pass Verdict:
1. DUT sends a node_desc_rsp frame with the Stack Compliance revision field in the server flags bitmap properly
   set to the stack revision being supported by the device. For example, in revision 21, the server flags would
   be 0x2A00 if none of the other server flags are set.

Fail Verdict:
1. DUT does not respond to node_desc_req or Stack Compliance revision field within the server flags bitmap does not match the stack revision supported by the device.

Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.
