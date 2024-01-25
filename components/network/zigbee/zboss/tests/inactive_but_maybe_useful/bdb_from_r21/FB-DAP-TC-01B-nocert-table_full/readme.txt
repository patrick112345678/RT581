FB-PRE-TC-07: INITIATOR: Binding table size and persistency

This test verifies the size of the binding table on an initiator device 
(which greater than or equal to the sum of the cluster instances, supported 
on each device of the node that are initiators of operational transactions).

Preparatory steps:

1. THr1 and DUT are operational on the same network (bdbNodeIsOnANetwork = TRUE).
2. THr1 gets a list of active endpoints of the DUT, and for each of the active endpoints, a Simple Descriptor.
   The THr1 determines the total number N and a list of clusters that are initiators of operational transactions.

Test procedure:

1. THr1 checks the content of the binding table of the DUT 
   THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00
2. DUT responds with the content of its binding table.
   DUT unicasts to the THr1 a correctly formatted Mgmt_Bind_rsp command.
3. THr1 fills binding table of the DUT with N entries.
   For each of the (N-M) Bind_req, DUT responds with Bind_rsp with Status = SUCCESS.
4. THr1 checks the content of the binding table of the DUT.
   THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.
5. DUT responds with the content of its binding table.
6. Persistent storage of the binding table.
   Power off DUT. Power DUT back on.
7. THr1 unicasts to the DUT a Mgmt_Bind_req with StartIndex = 0x00.
8. DUT responds with the content of its binding table.
