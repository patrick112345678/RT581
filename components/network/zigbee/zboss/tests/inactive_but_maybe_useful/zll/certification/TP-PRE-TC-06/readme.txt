TP-PRE-TC-06: Frequency agility
This test verifies the frequency agility procedure. A typical scenario might be if interference on the
existing channel is suspected to be a problem, the user may wish to try moving the network to a
different channel.
The test verifies the operation of the initiating end device, ZED1, and the non-factory new router, ZR1.
When running the test the DUT must take the role defined in its PICS. The other role must be played
by a golden unit or test harness.

Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR1, ZR2 - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).

(pre steps)
1.  Start ZR2 and ZED1 as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR responds with ZLL.Commissioning.Scan response.
4.  Logs of both ZR1 and ZED1 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
5.  ZED1 starts Start New Network procedure with all needs parameters
6.  ZR2 start new network and response ZED1.
7.  ZED1 rejoin this network.
8.  Finish ZR2 & ZED1.

1a  Start ZR1 as not factory new ZLL devices.
1b  ZR1 broadcasts a ZDO device_annce command frame.
2a  Start ZED1 as not factory new ZLL devices.
2b  ZED1 unicasts a NWK rejoin request command frame to ZR1.
2c  ZR1 unicasts a NWK rejoin response command frame to ZED1.
2d  ZED1 broadcasts a ZDO device_annce command frame.

(ZED1 instigates the channel change procedure)
3   ZED1 broadcasts a ZDO Mgmt_NWK_Update_req command frame to all RxOnWhenIdle devices.
4   On receipt of the request to change channel, ZR1 updates its NIB and executes its 
    channel change procedure.
5a  ZED1 unicasts a NWK rejoin request command frame to ZR1.
5b  ZR1 unicasts a NWK rejoin response command frame to ZED1.

(Turn off ZR1 and add ZR2 to the network with ZED1)
6a  ZR1 is powered off. ZR2 is powered on.
6b  ZED1 and ZR2 execute the touchlink procedure.
    ZED1 may optionally request ZR2 to identify in some way.
7   ZED1 broadcasts ZDO Mgmt_NWK_Update_req command frame addressed to all
    RxOnWhenIdle devices.

(Turn off ZR2 and add ZR1 back on the network with ZED1)
8a  ZR2 is powered off. ZR1 is powered on.
9a  ZED1 broadcasts up to five scan request inter-PAN command frames on the first
    ZLL primary channel and possibly broadcasts one on each of the remaining
    primary ZLL channels.
9b  ZR1 unicasts a scan response inter-PAN command frame to ZED1.
    This frame contains (amongst other fields) the value of its
    nwkUpdateId attribute.
10  ZED1 unicasts a network update request inter-PAN command frame to ZR1.
11a ZED1 broadcasts up to five scan request inter-PAN command frames on the first
    ZLL primary channel and possibly broadcasts one on each of the remaining
    primary ZLL channels.
11b ZR1 unicasts a scan response inter-PAN command frame to ZED1.
    This frame should be sent on the new channel, adjusted at step 10.

12  ZED1 and ZR1 complete the touchlink operation.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
