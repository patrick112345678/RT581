TP/PRO/BV-39 Frequency Agility – Channel Changeover – ZR
Verify that the DUT acting as a ZR behaves as appropriate during channel changeover.

gZC
DUT ZR (0x00 00 00 01 00 00 00 00)

Test procedure:
1. By implementation specific means gZC (Network Channel Manager) issues Mgmt_NWK_Update_req with 
channel mask for channel m (Different than the one network is on), where n=present channel.
2. After expiry of nwkNetworkBroadcastDeliveryTime gZC shall unicast a Buffer Test Request (0x001d) command to DUT ZR1.

Pass verdict:
1) gZC shall broadcast a Mgmt_NWK_Update_req with
    ChannelMask = m (Different than n)
    ScanDuration = 0xfe
2) gZC shall unicast a Buffer Test Request (0x001d) to DUT ZR1
3) DUT ZR1 shall unicast a Buffer Test Response (0x0054) to gZC

Fail verdict:
1) gZC does not broadcast a Mgmt_NWK_Update_req with
    ChannelMask = n+3
    ScanDuration = 0xfe
2) gZC does not unicast a Buffer Test Request (0x001d) to DUT ZR1
3) DUT ZR1 does not unicast a Buffer Test Response (0x004c) to gZC




