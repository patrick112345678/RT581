TP-PRE-TC-03:  Network join – ZED not factory new, ZR factory new
This test verifies the network join procedure. A typical scenario might be adding a new lamp to an
existing lighting network.
The test verifies the operation of the initiating end device, ZED1, and the factory new router, ZR1.
When running the test the DUT must take the role defined in its PICS. The other role must be played
by a golden unit or test harness.


Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR1, ZR2 - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).


1.  Start ZR2 and ZED1 as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR2 responds with ZLL.Commissioning.Scan response.
4.  Logs of both ZR1 and ZED1 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
5.  ZED1 starts Start New Network procedure with all needs parameters
6.  ZR2 start new network and response ZED1.
7.  ZED1 rejoin this network.
8.  Finish ZR2 & ZED1.
9.  Start ZR1 as factory new ZLL devices.
9.  Start ZED1 as not factory new ZLL devices.
10. ZED1 send rejoin - not response.
11. ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
12. ZR1 responds with ZLL.Commissioning.Scan response.
13. ZED1 selects ZR1 as its target. ZED1 transmits a network join router request to ZR1, 
    and enables its receiver for at least aplcRxWindowDuration seconds.
14. ZR1 responds to the join end device request and sets NIB values.
15. ZR1 attempts to rejoin its new network, changing channel if required.
16. ZED1 receives the response to its join end device request from ZED2 and stores NIB values.
17. ZR1 broadcast Device_Annce with ZED1 parameters


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
