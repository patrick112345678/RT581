FB-PRE-TC-07: INITIATOR: Binding table size and persistency

This test verifies the size of the binding table on an initiator device 
(which greater than or equal to the sum of the cluster instances, supported 
on each device of the node that are initiators of operational transactions).

Preparatory steps:

1. THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE).
2. THr1 gets a list of active endpoints of the DUT, and for each of the active endpoints, a Simple Descriptor.
   The THr1 determines the total number N and a list of clusters that are initiators of operational transactions.

Test procedure:
Binding table size
1	THr1 checks the content of the binding table of the DUT:
THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = M, whereby M may be 0x00;
StartIndex = 0x00;
BindingTableList present if M > 0x00;
BindingTableListCount set accordingly.
if the binding table is originally empty, then Mgmt_Bind_rsp carrying Status=UNSUPPORTED_ATTRIBUTE and no other fields after it is also acceptable.

2	THr1 fills binding table of the DUT, so that it contains N entries:
THr1 sends (N-M) Bind_req commands, each with:
SrcAddress being the IEEE address of the DUT, 
SrcEndp – being the initiator endpoint X of the DUT, 
ClusterID Y – being the initiator application clusters on that endpoint X;
DstAddrMode = 0x03, 
DstAddress being the IEEE address of the THr1 
DstEndp being a unique endpoint value from the range 0x01-0xf0. 	For each of the (N-M) Bind_req, DUT responds with Bind_rsp with Status = SUCCESS.
DUT does NOT send Bind_rsp with Status = TABLE_FULL.

3	THr1 checks the content of the binding table of the DUT:
THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = N;
StartIndex = 0x00;
BindingTableList present;
BindingTableListCount set accordingly.
Persistent storage of the binding table

4	Power off DUT.
Power DUT back on.
THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = N;
StartIndex = 0x00;
BindingTableList present and carrying content identical as in step 3;
BindingTableListCount set accordingly.

Verification:
1	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = M, whereby M may be 0x00;
StartIndex = 0x00;
BindingTableList present if M > 0x00;
BindingTableListCount set accordingly.
if the binding table is originally empty, then Mgmt_Bind_rsp carrying Status=UNSUPPORTED_ATTRIBUTE and no other fields after it is also acceptable.

2	For each of the (N-M) Bind_req, DUT responds with Bind_rsp with Status = SUCCESS.
DUT does NOT send Bind_rsp with Status = TABLE_FULL.

3	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = N;
StartIndex = 0x00;
BindingTableList present;
BindingTableListCount set accordingly.

4	DUT responds with the content of its binding table:
DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command (ClusterID=0x8033), with:
Status=SUCCESS, 
BindingTableEntries = N;
StartIndex = 0x00;
BindingTableList present and carrying content identical as in step 3;
BindingTableListCount set accordingly.

Additional info:
 To run test type ./runng.sh <dut_role>, where
 <dut_role> can be zc, zr or zed
 
