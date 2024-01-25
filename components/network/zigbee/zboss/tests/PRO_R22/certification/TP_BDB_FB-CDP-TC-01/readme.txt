5.4.1 FB-CDP-TC-01: Bind_req and Unbind_req reception test cases
This test verifies the behavior during binding creation and removal via the Bind_req/Unbind_req primitives.
When running the test the DUT must take the role defined in its PICS.  The other role must be played by a test harness.


Required devices:
DUT - DUT, supporting binding table; ZC, ZR or ZED, according to device role

THr1 - TH ZR; The THr1 is capable of sending Bind_req/Unbind_req (also malformed), and Mgmt_Bind_req.
THr2 - TH ZR to be bound to the DUT
The THr2 has at least two endpoints, T1 and T2, which both have the same cluster, matching one of the initiator clusters of the DUT


Preparatory steps:
P1	All devices are operational on the same network (bdbNodeIsOnANetwork = TRUE).
P2	THr1 reads out the initial status of binding table of the DUT, by sending Mgmt_Bind_req with StartIndex = 0x00. 
P3	Conditional on DUT having reportable attributes in the PICS-indicated initiator cluster (type 2 cluster):
THr1 reads the default reporting configuration of the PICS-indicated reportable attribute of the PICS-indicated initiator cluster Y of the initiator endpoint X of the DUT.

Note: the selected reportable attribute may be used for testing the operation of the binding table in test step 5 below. Alternatively, for type 1 initiator clusters, command transmission can be triggered on the DUT in test step 5 below.




Test Procedure:
Optional: Remove default bindings, if any
0a	Conditional on the DUT supporting default bindings:
If possible, THr1 removes all the default bindings.
Other removal methods, if available, e.g. local trigger at the DUT, may be used as well. 	For each default binding table entry of the DUT, the THr1 unicasts to the DUT an Unbind_req command (ClusterID=0x0022), with the fields set exactly as in the to-be-removed binding table entry.
0b	Conditional on 0a: 
DUT confirms removal of each default bindings.	Upon reception of each Unbind_req, the DUT unicasts to the THr1 an Unbind_rsp with Status=SUCCESS.
0c	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.
0d	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below:
Status=SUCCESS, BindingTableEntries = StartIndex = BindingTableListCount = 0x00 and BindingTableList field not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field omitted.
Create unicast binding
Note: The following steps assume initially empty binding table. if DUT supports default bindings and they were not removed in step 0, their presence shall be tablen into account in the following steps.  
1a	THr1 creates a unicast binding for one of DUT’s initiator endpoints X and clusters Y. 	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X being (one of) DUT’s initiator endpoint(s), ClusterID Y being one of DUT’s initiator clusters on the endpoint X, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
1b	DUT responds with Bind_rsp with Status=SUCCESS.	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
Create another unicast binding for the same DUT endpoint and cluster
2a	THr1 creates another unicast binding for the same DUT’s initiator endpoint X and cluster Y. 	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y on endpoint X, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T2.
2b	DUT responds with Bind_rsp with Status=SUCCESS.	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
Create groupcast binding for the same DUT endpoint and cluster
3a	THr1 creates a groupcast binding for the same DUT’s initiator endpoint X and cluster Y. 	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y on endpoint X, DstAddrMode = 0x01, DstAddress G.
3b	DUT responds with Bind_rsp with Status=SUCCESS.	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
4a	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
4b	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x00, BindingTableListCount = 0x03 or lower, and BindingTableList carrying each of the entries created in steps 1a, 2a and 3a.
If the response does not contain 3 entries, i.e. the value of the BindingTableEntries field is higher than the value of the BindingTableListCount, step 4a and 4b shall be repeated, until all binding table entries are examined.
Check operation of three different bindings for the same DUT endpoint and cluster
5	DUT is triggered to initiate communication from the bound cluster Y on the bound endpoint X.	DUT sends a unicast message from endpoint X, cluster Y to THr2 endpoint T1, another unicast message from endpoint X, cluster Y to THr2 endpoint T2, and a groupcast message to APS group G. 
Negative unbind test: non-existing binding (wrong source address)
6a	THr1 sends an unbind command with source address NOT of the DUT.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress NOT equal to the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
6b	DUT responds with NOT_SUPPORTED.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NOT_SUPPORTED.
Negative unbind test: non-existing binding (wrong source endpoint)
7a	THr1 sends an unbind command with source endpoint NOT equal to X.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp NOT equal to X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
7b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
Negative unbind test: non-existing binding (wrong cluster)
8a	THr1 sends an unbind command with cluster NOT equal to Y.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID NOT equal to Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
8b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
Negative unbind test: non-existing binding (wrong group address)
9a	THr1 sends an unbind command with wrong group address.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress NOT equal to G.
9b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
Negative unbind test: non-existing binding (wrong unicast destination address)
10a	THr1 sends an unbind command with wrong unicast destination.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress NOT equal to IEEE address of THr2 and DstEndp T1.
10b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
Negative unbind test (wrong destination endpoint):
11a	THr1 sends an unbind command with wrong destination endpoint.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp NOT equal to either T1 or T2.
11b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
DEFERRED (steps 12-13): Negative unbind test: unbind received in broadcast 
12a	THr1 sends a broadcast unbind command.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
12b	DUT does NOT respond. DUT does NOT remove the entry.	DUT does NOT respond. DUT does NOT remove the entry.
13a	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
13b	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x00, BindingTableListCount = 0x03, and BindingTableList carrying each of the entries created in steps 1a, 2a and 3a.
Mgmt_Bind_req tests
14a	StartIndex != 0x00:
THr1 sends Mgmt_Bind_req with StartIndex = 0x02.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x02.
14b	DUT responds with the content of its binding table starting at the StartIndex.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x02, BindingTableListCount = 0x01, and BindingTableList carrying one of the entries created in steps 1a, 2a and 3a.
14c	StartIndex > BindingTableEntries:
THr1 sends Mgmt_Bind_req with StartIndex = 0x05.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x05.
14d	DUT responds with the content of its binding table starting at the StartIndex.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x05, BindingTableListCount = 0x00, and BindingTableList field not present.
Unbind the groupcast binding
15a	THr1 sends an unbind command for the groupcast binding.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress being G.
15b	DUT responds with SUCCESS.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=SUCCESS.
15c	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
15d	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x02, StartIndex = 0x00, BindingTableListCount = 0x02, and BindingTableList carrying the two unicast binding table entries for THr2 endpoint T1 and T2.
Negative unbind test (non-existing binding)
16a	THr1 sends an unbind command for the groupcast binding.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress being G.
16b	DUT responds with NO_ENTRY.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= NO_ENTRY.
Unbind the unicast bindings
17a	THr1 sends an unbind command for the unicast binding.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of the THr2 and DstEndp T2.
17b	DUT responds with SUCCESS.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= SUCCESS.
17c	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
17d	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x01, StartIndex = 0x00, BindingTableListCount = 0x01, and BindingTableList carrying one unicast binding table entry for THr2 endpoint T1.
17e	THr1 sends an unbind command for the unicast binding.	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of the THr2 and DstEndp T1.
17f	DUT responds with SUCCESS.	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= SUCCESS.
17g	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
17h	DUT responds with the content of its binding table.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below:
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.
DEFERRED (steps 20-21): Negative test: Bind_req received in broadcast
20a	THr1 broadcasts a Bind_req. 	THr1 broadcasts the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
20b	DUT does NOT create the binding.	DUT does NOT respond with Bind_rsp.
21a	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
21b	DUT responds with the content of its binding table and it is empty.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below:
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.
Negative test: binding for another device if Primary binding table cache is NOT supported
22a	THr1 attempts to create a binding in the DUT’s binding cache. 	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress NOT being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
22b	DUT responds with Status NOT_SUPPORTED.	DUT unicasts to the THr1 a ZDO Bind_rsp command (ClusterID=0x8021), with Status= NOT_SUPPORTED.
22c	THr1 checks the content of the binding table of the DUT.	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
22d	DUT responds with the content of its binding table and it is empty.	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below: 
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.

Verification:
1a	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X being (one of) DUT’s initiator endpoint(s), ClusterID Y being one of DUT’s initiator clusters on the endpoint X, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
1b	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
2a	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y on endpoint X, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T2.
2b	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
3a	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y on endpoint X, DstAddrMode = 0x01, DstAddress G.
3b	DUT unicasts to the THr1 a Bind_rsp with Status=SUCCESS.
4a	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
4b	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x00, BindingTableListCount = 0x03 or lower, and BindingTableList carrying each of the entries created in steps 1a, 2a and 3a.
If the response does not contain 3 entries, i.e. the value of the BindingTableEntries field is higher than the value of the BindingTableListCount, step 4a and 4b shall be repeated, until all binding table entries are examined.
5	DUT sends a unicast message from endpoint X, cluster Y to THr2 endpoint T1, another unicast message from endpoint X, cluster Y to THr2 endpoint T2, and a groupcast message to APS group G. 
6a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress NOT equal to the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
6b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NOT_SUPPORTED.
7a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp NOT equal to X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
7b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
8a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID NOT equal to Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
8b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
9a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress NOT equal to G.
9b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
10a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress NOT equal to IEEE address of THr2 and DstEndp T1.
10b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
11a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp NOT equal to either T1 or T2.
11b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=NO_ENTRY.
12a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of THr2 and DstEndp T1.
12b	DUT does NOT respond. DUT does NOT remove the entry.
13a	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
13b	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x00, BindingTableListCount = 0x03, and BindingTableList carrying each of the entries created in steps 1a, 2a and 3a.
14a	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x02.
14b	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x02, BindingTableListCount = 0x01, and BindingTableList carrying one of the entries created in steps 1a, 2a and 3a.
14c	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x05.
14d	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x03, StartIndex = 0x05, BindingTableListCount = 0x00, and BindingTableList field not present.
15a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress being G.
15b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status=SUCCESS.
15c	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
15d	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x02, StartIndex = 0x00, BindingTableListCount = 0x02, and BindingTableList carrying the two unicast binding table entries for THr2 endpoint T1 and T2.
16a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x01, DstAddress being G.
16b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= NO_ENTRY.
17a	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of the THr2 and DstEndp T2.
17b	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= SUCCESS.
17c	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
17d	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with Status=SUCCESS, BindingTableEntries = 0x01, StartIndex = 0x00, BindingTableListCount = 0x01, and BindingTableList carrying one unicast binding table entry for THr2 endpoint T1.
17e	THr1 unicasts to the DUT the ZDO Unbind_req command (ClusterID=0x0022), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being IEEE address of the THr2 and DstEndp T1.
17f	DUT unicasts to the THr1 a ZDO Unbind_rsp command (ClusterID=0x8022), with Status= SUCCESS.
17g	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
17h	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below:
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.
20a	THr1 broadcasts the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
20b	DUT does NOT respond with Bind_rsp.
21a	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
21b	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below:
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.
22a	THr1 unicasts to the DUT the ZDO Bind_req command (ClusterID=0x0021), with SrcAddress NOT being the IEEE address of the DUT, SrcEndp X, ClusterID Y, DstAddrMode = 0x03, DstAddress being the IEEE address of the THr2 and DstEndp T1.
22b	DUT unicasts to the THr1 a ZDO Bind_rsp command (ClusterID=0x8021), with Status= NOT_SUPPORTED.
22c	THr1 unicasts to the DUT a Mgmt_Bind_req, with StartIndex = 0x00.
22d	DUT unicasts to the THr1 a Mgmt_Bind_rsp command (ClusterID=0x8033), with either of the below: 
Status=SUCCESS, BindingTableEntries = 0x00, StartIndex = 0x00, BindingTableListCount = 0x00, and BindingTableList not present;
Status=UNSUPPORTED_ATTRIBUTE and all fields after the Status field not present.




Additional info:
to run test type - runng <dut_role>
where dut_role can be zc, zed or zr
e.g: runng zc
