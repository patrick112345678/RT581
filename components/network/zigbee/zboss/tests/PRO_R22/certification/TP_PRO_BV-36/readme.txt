TP/PRO/BV-36 Frequency Agility – Network Channel Manager - ZC
Verify that the DUT acting as a ZC can operate as default Network Channel Manager.

DUT ZC
gZR1

Test procedure:
1. gZR1 shall broadcast (0xfffd) a System_Server_Discovery_req command with the ServerMask set to Network Manager (bit 6 = 1).

Pass verdict:
1) gZR1 broadcasts a System_Server_Discovery_req command with the ServerMask set to Network Manager (bit 6 = 1)
2) DUT ZC shall unicast a System_Server_Discovery_rsp command back to gZR1 with Status set to SUCCESS and ServerMask set to Network Manager.

Fail verdict:
1) gZR1 does not broadcast a System_Server_Discovery_req command with the ServerMask set to Network Manager (bit 6 = 1)
2) DUT ZC does not unicast a System_Server_Discovery_rsp command back to gZR1 with Status set to SUCCESS and ServerMask set to Network Manager.

Comments:
to execute test start runng.sh
To check results, analyse log files, on success zdo_zr.log contains
"system_server_discovery received, status: OK"
on fail "ERROR receiving system_server_discovery"
