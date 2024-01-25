This test is a custom one checking start new network with two ZLL devices.

ZR  - ZLL router, namely Non-Color Scene Controller (single endpoint).
ZED - ZLL ED, namely Dimmable Light. The device is address-assignment-capable
      (i. e. compiled with ZB_ROUTER_ROLE defined).

1.  Both ZR and ZED start as factory new ZLL devices. Both of them check that they
    are in initial state and idle (i. e. not connected to any network). Both
    of them are limited to use the first ZLL primary channel only.
2.  ZED starts Device Discovery procedure by sending ZLL.Commissioning.Scan request.
3.  ZR responds with ZLL.Commissioning.Scan response.
4.  Logs of both ZR and ZED are being analyzed for appropriate results (i. e.
    no errors in trace output, all packets sent successfully, appropriate
    packets received).
5.  ZED starts Start New Network procedure with all needs parameters
6.  ZR start new network and response ZED.
7.  ZED rejoin this network.

To start test in Linux with network simulator:
- run ./run.sh
- analyze output

To diagnose errors:
- view .pcap file by Wireshark
- analyze trace logs
