TP/R21/BV-27 Test for mgmt_leave_req
Objective: Demonstrate the device’s ability to leave the network when requested, or remove one of
           its end-device children upon request.

Initial Conditions:

DUT - ZR/ZC/ZED

Devices: DUT, gZR, gZED


Test Procedure:
1 Make sure that DUT and gZR (test harness) are on the same network and able to communicate, e.g.
  - if DUT is a ZC, join gZR to DUT ZC;
  - if DUT is a ZR, let gZR form a distributed security network and let DUT ZR join;
  - if DUT is a ZED, let gZR form a distributed security network and let DUR ZED join.
2 If DUT is a ZR or ZC, let gZED join at DUT ZR/ZC else skip this step (PASS)
3 Let gZR send a mgmt_leave_req to DUT, with DeviceAddress = 0x0000 0000 0000 0000,
  rejoin = 1, remove children = 0 (Criteria 1 – 3)
4 If DUT is not a ZR or ZC, skip this step (PASS) and advance to step 5.
  Let gZR send a mgmt_leave_req to DUT, with DeviceAddress = EUI-64(gZED1), rejoin = 0, remove children = 0 (Criterion 4)
4a Let gZR query DUT’s complete neighbour table by issuing as may mgmt_lqi_req commands as necessary. (Criterion 5)
5 Let gZR send a mgmt_leave_req to DUT, with DeviceAddress = 0x0000 0000 0000 0000,
  rejoin = 0, remove children = 0 (Criterion 6)


Pass Verdict:
1 (DUT ZR/ZED ONLY) DUT broadcasts a NWK leave command frame, radius = 1, with the request flag = ‘0’, the rejoin flag = ‘1’.
1 (DUT ZC ONLY) DUT ZC ignores the NWK leave command.
2 (DUT ZR/ZED ONLY) DUT performs an active scan and is able to complete a secure or insecure rejoin.
                    Note: After the rejoin, the device may have changed its network short address.
3 (DUT ZR/ZED ONLY) There is evidence that DUT is still on the network, e.g. ZR/ZC emit valid link status
                    messages, ZED responds to buffer test request.
4 (DUT ZR/ZC ONLY) DUT sends a NWK leave command unicast addressed to gZED1, with the request flag = ‘1’, rejoin flag = ‘0’.
5 (DUT ZR/ZC ONLY) DUT has altered the neighbor table so that gZED1 is no longer present in the table, or if present the relationship indicates previous child (0x04).
6 (DUT ZR/ZED ONLY) DUT broadcasts a NWK leave command frame, radius = 1, with the request flag = ‘0’, the rejoin flag = ‘0’.
7 (DUT ZR/ZED ONLY) There is evidence that DUT has left the network, e.g. ZR does not emit link status messages,
                    ZED does not respond to buffer test request and does not attempt to poll. It may start an active
                    scan for a new  network to join. It does not attempt to rejoin the same network as before.
7 (DUT ZC ONLY) DUT ZC ignores the NWK leave command.



Fail Verdict:
1 (DUT ZR/ZED ONLY) DUT does not broadcast a NWK leave command frame, or radius is not 1, or
                    request flag is ‘1’, or rejoin flag is ‘0’.
1 (DUT ZC ONLY) DUT ZC broadcasts a NWK leave command frame or there is other evidence that DUT ZC has left the network.
2 (DUT ZR/ZED ONLY) DUT does not perform active scans or does not send rejoin request with or without NWK security,
                    or otherwise fails to rejoin the network.
3 (DUT ZR/ZED ONLY) DUT is no longer on the network, e.g. it does not respond to buffer test
                    request or, as a ZR/ZC does not emit link status messages.
4 (DUT ZR/ZC ONLY) DUT does not send an NWK leave command, or it is not addressed to gZED1, or the request flag is ‘0’, or the rejoin flag is ‘1’.
5 (DUT ZR/ZC ONLY) DUT still lists gZED as one of its child devices.
6 (DUT ZR/ZED ONLY) DUT does not broadcast a NWK leave command frame, or radius is not 1, or
                    request flag is ‘1’, or rejoin flag is ‘1’.
7 (DUT ZR/ZED ONLY) There is evidence that DUT did not leave the network, e.g. ZR emits link status
                    messages, ZED responds to buffer test request or attempts to poll. DUT attempts to rejoin
                    the same network as before.
7 (DUT ZC ONLY) DUT ZC broadcasts a NWK leave command frame or there is other evidence that DUT ZC has left the network.




Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc, zr or zed: i.e. ./runng zc to start test with DUT as ZC.


./runng.sh zc
tp_r21_bv-27_dutzc.c
tp_r21_bv-27_gzr_c.c
tp_r21_bv-27_gzed.c


./runng.sh zr
tp_r21_bv-27_gzr_d.c
tp_r21_bv-27_dutzr.c
tp_r21_bv-27_gzed.c


./runng.sh zed
tp_r21_bv-27_gzr_d.c
tp_r21_bv-27_dutzed.c
