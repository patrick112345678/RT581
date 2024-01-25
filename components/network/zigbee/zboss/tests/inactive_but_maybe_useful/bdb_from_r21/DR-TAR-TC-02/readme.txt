5.4.1 FB-CDP-TC-01: Bind_req and Unbind_req reception test cases
This test verifies the behavior during binding creation and removal via the Bind_req/Unbind_req primitives.
When running the test the DUT must take the role defined in its PICS.  The other role must be played by a test harness.


Required devices:
DUT - DUT, supporting binding table; ZC, ZR or ZED, according to device role

THr1 - TH ZR; The THr1 is capable of sending Bind_req/Unbind_req (also malformed), and Mgmt_Bind_req.
THr2 - TH ZR to be bound to the DUT
The THr2 has at least two endpoints, T1 and T2, which both have the same cluster, matching one of the initiator clusters of the DUT


Preparatory steps:
P1 - All devices are operational on the same network (bdbNodeIsOnANetwork = TRUE).
P2 - THr1 reads out the initial status of binding table of the DUT, by sending Mgmt_Bind_req with StartIndex = 0x00. 
P3 - THr1 reads the default reporting configuration of the reportable attributes of the initiator endpoint(s) of the DUT.



Test Procedure:
 - Optional: Remove default bindings, if any
   - Conditional on the DUT supporting default bindings:
     THr1 removes all the default bindings.
   - THr1 checks the content of the binding table of the DUT.
 - Create unicast binding
   - THr1 creates a unicast binding for one of DUT’s initiator endpoints X and clusters Y. 
 - Create another unicast binding for the same DUT endpoint and cluster
   - THr1 creates another unicast binding for the same DUT’s initiator endpoint X and cluster Y.
 - Create groupcast binding for the same DUT endpoint and cluster
   - THr1 creates a groupcast binding for the same DUT’s initiator endpoint X and cluster Y.
   - THr1 checks the content of the binding table of the DUT.
 - Check operation of three different bindings for the same DUT endpoint and cluster
   - DUT is triggered to initiate communication from the bound cluster Y on the bound endpoint X.
 - Negative unbind test: non-existing binding (wrong source address)
   - THr1 sends an unbind command with source address NOT of the DUT.
 - Negative unbind test: non-existing binding (wrong source endpoint)
   - THr1 sends an unbind command with source endpoint NOT equal to X.
 - Negative unbind test: non-existing binding (wrong cluster)
   - THr1 sends an unbind command with cluster NOT equal to Y.
 - Negative unbind test: non-existing binding (wrong group address)
   - THr1 sends an unbind command with wrong group address.
 - Negative unbind test: non-existing binding (wrong unicast destination address)
   - THr1 sends an unbind command with wrong unicast destination.
 - Negative unbind test (wrong destination endpoint):
   - THr1 sends an unbind command with wrong destination endpoint.
 - Negative unbind test: unbind received in broadcast
   - THr1 sends a broadcast unbind command.
   - THr1 checks the content of the binding table of the DUT.
 - Mgmt_Bind_req tests
   - StartIndex != 0x00: THr1 sends Mgmt_Bind_req with StartIndex = 0x02.
   - StartIndex > BindingTableEntries: THr1 sends Mgmt_Bind_req with StartIndex = 0x05.
 - Unbind the groupcast binding
   - THr1 sends an unbind command for the groupcast binding.
   - THr1 checks the content of the binding table of the DUT.
 - Negative unbind test (wrong destination address mode)
   - THr1 sends an unbind command for the groupcast binding.
 - Unbind the unicast bindings
   - THr1 sends an unbind command for the groupcast binding.
   - THr1 checks the content of the binding table of the DUT.
   - THr1 sends an unbind command for the groupcast binding.
   - THr1 checks the content of the binding table of the DUT.
 - Negative test: binding request to wrong (inactive) DUT endpoint
   - THr1 creates a unicast binding for an endpoint NOT supported by the DUT.
 - Negative test: binding request for unsupported cluster
   - THr1 creates a unicast binding for a cluster NOT implemented on endpoint X of the DUT.
 - Negative test: Bind_req received in broadcast
   - THr1 broadcasts a Bind_req.
   - THr1 checks the content of the binding table of the DUT.
 - Negative test: binding for another device if Primary binding table cache is NOT supported
   - THr1 attempts to create a binding in the DUT’s binding cache.
   - THr1 checks the content of the binding table of the DUT.


Additional info:
to run test type - runng <dut_role>
where dut_role can be zc, zed or zr
e.g: runng zc
