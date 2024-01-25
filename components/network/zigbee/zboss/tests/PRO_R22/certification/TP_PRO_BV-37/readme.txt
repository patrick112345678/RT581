TP/PRO/BV-37 Frequency Agility – Receipt of a Mgmt_NWK_Update_req – ZR
Verify that the DUT acting as a ZR correctly responds to a Mgmt_NWK_Update_req from the Network Channel Manager.

gZC
DUT ZR1

Test procedure:
1. gZC shall unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x06;
   ScanCount = 0x01; to DUT ZR1.
2. gZC shall unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x05;
   ScanCount = 0x01; to DUT ZR1.

Pass verdict:
1) gZC shall unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x06;
   ScanCount = 0x01; to DUT ZR1.
2) DUT ZR1 shall unicast a Mgmt_NWK_Update_notify command with Status = INVALID REQUEST
3) gZC shall unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x05;
   ScanCount = 0x01; to DUT ZR1.
4) DUT ZR1 shall unicast a Mgmt_NWK_Update_notify command with
   Status = SUCCESS;
   ScannedChannels = 0x07fff800;
   TotalTransmissions = 0x0000;
   TransmissionFailures = 0x0000;
   ScannedChannelsListCount = 0xff;
   EnergyValues = (As appropriate)

Fail verdict:
1) gZC does not unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x06;
   ScanCount = 0x01; to DUT ZR1.
2) DUT ZR1 does not unicast a Mgmt_NWK_Update_notify command with Status = INVALID REQUEST
3) gZC does not unicast a Mgmt_NWK_Update_req command with
   ChannelMask = 0x07fff800;
   ScanDuration = 0x05;
   ScanCount = 0x01; to DUT ZR1.
4) DUT ZR1 does not unicast a Mgmt_NWK_Update_notify command with
   Status = SUCCESS;
   ScannedChannels = 0x07fff800;
   TotalTransmissions = 0x0000;
   TransmissionFailures = 0x0000;
   ScannedChannelsListCount = 0xff;
   EnergyValues = (As appropriate);

Comments:
To execute test, start runng.sh
Analyse log files to check results, run.sh script writes "DONE. TEST PASSED!!!"
on success and "ERROR. TEST FAILED!!!" on error.
