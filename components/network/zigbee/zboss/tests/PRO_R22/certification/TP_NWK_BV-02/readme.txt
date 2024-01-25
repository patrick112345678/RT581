11.2 TP/NWK/BV-02 ZR-ZDO-APL RX Join/Leave
Verify that the device ZDO Network Manager functionality is capable of a network join or
leave, and verify the status.

gZC
DUT ZR

gZC
PANid= 0x1AAA
Logical Address = 0x0000
0xaa aa aa aa aa aa aa aa

ZR:
DUT
Will be assigned logical address
0x00 00 00 01 00 00 00 00

Initial Conditions
1. Reset nodes
2. Set gZC under target stack profile, gZC as coordinator starts a PAN = 0x1AAA network
3. Set gZC to allow for devices to join the network.


Test procedure:
1. DUT ZR performs a start-up.
2. Based on existing policy, DUT ZR performs NWK Join. Join to ExtendedPANId=0xaaaaaaaaaaaaaaaa;
3. DUT ZR performs a NWK LEAVE (self leave, address=NULL,rejoin=FALSE);

!!NOTE: Test procedure says "4) ZR shall issue a self-induced NWK LEAVE command frame to gZC, 	with 
	the IEEE 64-bit address set to itself, with reason = 0x02.", but no "reason" parameter in LEAVE! This is problem in the TC document.

4. DUT ZR shall issue a Test Buffer Request to gZC;
5. Based on existing policy, DUT ZR performs NWK Join.
6. gZC performs a LEAVE, where NLME-LEAVE.request(DeviceAddress=IEEE of ZR, rejoin=FALSE; remove children=FALSE); 
For ZigBee Pro, gZC sends Mgmt_Leave.request with DevAddr=all zero, DstAddr=ZR, rejoin=FALSE, remove children=FALSE;
7. DUT ZR shall issue a Test Buffer Request to ZC.

Pass verdict:
1) ZR DUT shall issue a MLME_Beacon_Request MAC command frame.
2) Based on the active scan result, ZR DUT shall complete the MAC association sequence.
3) gZC is able to receive the association request (places the ZR 64-bit address in ts neighbor table).
4) ZR shall issue a self-induced NWK LEAVE command frame to gZC, with the IEEE 64-bit address set to itself, with reason = 0x02.
5) ZR returns error on attempting to issue the Test Buffer Request.
6) After ZR joins the network anew, gZC issues a NWK LEAVE to ZR. ZR shall not have the gZC as its coordinator.
7) ZR returns error on attempting to issue the Test Buffer Request.

Fail verdict:
1) ZR DUT does not issue a MLME_Beacon_Request MAC command frame
2) ZR does not perform the MAC association sequence
3) gZC does not put ZR in its neighbor table.
4) ZR does not perform a NWK leave sequence, where a NWK leave onfirmation command is not issued, and optionally, 
Disassociation  notification MAC command frame may be issued to the gZC.
5) ZR issues a Test Buffer request to the coordinator.
6) ZR MAC PIB still has address of gZC as its coordinator.


Comments:
Verify that the device ZDO Network Manager functionality is capable of a network join or
leave, and verify the status.

- DUT ZR performs a NWK LEAVE (self leave, address=NULL, rejoin=FALSE)
- Based on existing policy, DUT ZR performs NWK Join.
- gZC performs a LEAVE, where NLMELEAVE.
request(DeviceAddress=IEEE of ZR, rejoin=FALSE; remove children=FALSE);

to execute test start runng.sh
To check results, analyse log files, on success tp_nwk_bv-02_zr(pid).log contains
"Device STARTED OK" after leave.


Note that ZR sends broadcast LEAVE after receiving LEAVE from ZC - that is right.

ZR does not send data to ZC - ok.
Search in log for data_confirm after "###send_data to coordinator"
Trace is "zb_nlde_data_confirm: confirm with error state 0xc2, pass up buffer;"
