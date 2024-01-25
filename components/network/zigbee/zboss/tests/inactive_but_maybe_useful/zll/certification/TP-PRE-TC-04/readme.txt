TP-PRE-TC-04: Network join – ZED & ZR not factory new

This test verifies the network join procedure. A typical scenario might be adding to an existing lighting
network a lamp that had previously been used elsewhere, perhaps in a separate lighting network that
the user now wants to combine into a single larger network.
The test verifies the operation of the initiating end device, ZED1, and the non-factory new router, ZR1.
When running the test the DUT must take the role defined in its PICS. The other role must be played
by a golden unit or test harness.



Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.

Note: Need define NVRAM - see zb_config.h ZB_USE_NVRAM


ZED1, ZED2 - ZLL ED, namely Dimmable light (single endpoint).
ZR1, ZR2  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).


1.  Start ZR2 and ZED1 both as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR2 responds with ZLL.Commissioning.Scan response.
4.  Logs of both ZR2 and ZED1 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
5.  ZED1 starts Start New Network procedure with all needs parameters
6.  ZR2 start new network and response ZED1.
7.  ZED1 rejoin this network.
8.  Finish ZR2 and ZED1.

9.  Start ZR1 and ZED2 both as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
10. ZED2 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
11. ZR1 responds with ZLL.Commissioning.Scan response.
12. Logs of both ZR1 and ZED2 are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
13. ZED2 starts Start New Network procedure with all needs parameters
14. ZR1 start new network and response ZED2.
15. ZED2 rejoin this network.
16. Finish ZR1 and ZED2.

17. Start ZR1 as non factory new ZLL devices. ZR1 load connect parameters from NVRAM.
18. ZR1 send Device annonce.
19. Start ZED1 as non factory new ZLL devices. ZED1 load connect parameters from NVRAM.
20. ZED1 send rejoin to previous this network. There is no response.
21. ZED1 starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
22. ZR1 responds with ZLL.Commissioning.Scan response.
23. ZED1 selects ZR1 as its target. ZED1 transmits a join router request to ZR1,.
    and enables its receiver for at least aplcRxWindowDuration seconds.
24. ZR1 responds to the join router request and sets NIB values.
25. ZR1 send Leave broadcast message.
26. ZR1 send Device annonce.
27. ZR1 start new network and response ZED1.
28. ZED1 unicasts a NWK rejoin request command frame to ZR1.
29. ZR1 unicasts a NWK rejoin response command frame to ZED1.
30. ZED1 send Device annonce.


To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
