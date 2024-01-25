TP-PRE-TC-02: Network start – ZED factory new, ZR not factory new

This test verifies the network start procedure. A typical scenario might be a new remote control being
started for the first time, and wanting to take over control of an existing lamp that had previously been
used with an old remote control that is no longer available.
The test verifies the operation of the initiating end device, ZED1, and the non-factory new router, ZR1.
When running the test the DUT must take the role defined in its PICS. The other role must be played
by a golden unit or test harness.



Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED1,ZED2 - ZLL ED, namely Dimmable light (single endpoint).
ZR  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).


1.  Start ZR and ZED2 both as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED2 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR responds with ZLL.Commissioning.Scan response.
4.  Logs of both ZR and ZED2 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
5.  ZED2 starts Start New Network procedure with all needs parameters
6.  ZR start new network and response ZED.
7.  ZED2 rejoin this network.
8.  Finish ZR and ZED2.
9.  Start ZR as nob factory new and ZED1 as factory new ZLL devices. ZR load connect 
    parameters from NVRAM.
10. ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
11. ZR responds with ZLL.Commissioning.Scan response.
12.  Logs of both ZR and ZED2 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
13. ZED1 starts Start New Network procedure with all needs parameters
14. ZR send Leave broadcast message.
15. ZR start new network and response ZED.
16. ZR send Device annonce.
17. ZED1 rejoin this network.
18. ZED1 send Device annonce.

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
