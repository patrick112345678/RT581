TP-PRE-TC-05:  Network join – ZED1 not factory new, ZED2 factory new

This test verifies the network join procedure. A typical scenario might be adding an additional new
remote control to an existing lighting network.
The test verifies the operation of the initiating end device, ZED1, and a second factory new end device,
ZED2. When running the test the DUT must take the role defined in its PICS. The other role must be
played by a golden unit or test harness.


Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED1, ZED2 - ZLL ED, namely Dimmable light (single endpoint).
ZR2  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).


1.  Start ZR2 as factory new and ZED1 as not factory new ZLL devices. Both of them check that they
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
9.  Start ZED2 as factory new ZLL devices.
9.  Start ZED1 as not factory new ZLL devices.
10. ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
11. ZED2 responds with ZLL.Commissioning.Scan response.
12. ZED1 selects ZED2 as its target. ZED1 transmits a join end device request to ZED2, 
    and enables its receiver for at least aplcRxWindowDuration seconds.
13. ZED2 responds to the join end device request and sets NIB values.
14. ZED2 attempts to rejoin its new network, changing channel if required.
15. ZED1 receives the response to its join end device request from ZED2 and stores NIB values.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
