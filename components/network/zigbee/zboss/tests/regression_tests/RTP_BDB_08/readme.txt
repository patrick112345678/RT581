Bug 13794 - ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED behaviour
Bug 13792 - Possibility to cancel F&B on the initiator side  

RTP_BDB_08 - ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED signal comes with appropriate status, initiator can cancel finding and binding

Objective:

	To confirm that initiator can cancel finding and binding and the ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED signal 
	comes with appropriate status (success, cancel, alarm, error).

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
    2. Power on TH ZR
    3. Wait for DUT ZC start finding and binding and finish it with timeout
	4. Wait for DUT ZC fill own binding table through bind requests to itself
	5. Wait for TH ZR start finding and binding (as target)
	6. Wait for DUT ZC start finding and binding and finish it with error (binding table is full)
	7. Wait for DUT ZC print its local binding table to trace
	8. Wait for DUT ZC clear binding table
    9. Wait for DUT ZC start finding and binding (as initiator) and then cancel it
	10. Wait for DUT ZC start finding and binding and finish it with success result
	11. Wait for DUT ZC print its local binding table to trace

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3. DUT ZC starts finding and binding procedure (as initiator) for a short time
	3.1. DUT ZC sends Identify Query and gets timeout, any binding is not performed.
	3.2. DUT ZC receives signal ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED with ZB_ZDO_FB_INITIATOR_STATUS_ALARM status.

	The trace should contain string "BDB finding and binding initiator finished, status 2"

	4. DUT ZC sends 32 bind requests to itself and completely fills own binding table.

	5. TH ZR starts finding and binding as target

	6. DUT ZC tries to perform finding and binding.
	6.1. DUT ZC sends Identify Query, receives Identify Query Response.
	6.2. DUT ZC sends Simple Descriptor Request and receives Simple Descriptor Response, but does not perform any binding because table is overflow.
	6.3. DUT ZC receives signal ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED with ZB_ZDO_FB_INITIATOR_STATUS_ERROR status.

	The trace should contain strings:
		- BDB finding and binding initiator finished, status 3

	7. DUT ZC prints its local binding table to trace

	The trace should contain 32 strings "mgmt_bind_resp_cb: record - src endp 143, dst endp 143, cluster id XX", 
	where XX is some cluster ID.
	The trace should not contain such string with "src endp 143, dst endp 10" until next step

	8. DUT ZC clears own binding table

	9. DUT ZC starts finding and binding
	9.1. DUT ZC sends Identify Query and then cancel the procedure. 
	9.2. DUT ZC receives Identify Query Response, but any binding is not performed because of cancel.
	9.3 DUT ZC receives signal ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED with ZB_ZDO_FB_INITIATOR_STATUS_CANCEL status.

	The trace should contain string "BDB finding and binding initiator finished, status 1"

	10. DUT ZC performs finding and binding.
	10.1. DUT ZC sends Identify Query, receives Identify Query Response
	10.2. DUT ZC sends Simple Descriptor Request, receives Simple Descriptor Response and performs binding
	10.3. DUT ZC receives signal ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED with ZB_ZDO_FB_INITIATOR_STATUS_SUCCESS status.

	The trace should contain strings:
		- BDB finding and binding initiator finished, status 0

	11. DUT ZC prints its local binding table to trace
	
	The trace should contain strings:
		- mgmt_bind_resp_cb: record - src endp 143, dst endp 10, cluster id 3
		- mgmt_bind_resp_cb: record - src endp 143, dst endp 10, cluster id 0