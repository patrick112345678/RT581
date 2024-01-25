This test verifies the network start procedure. A typical scenario might be a new remote control and a
new lamp being started for the first time.
The test verifies the operation of the initiating end device, ZED1, and the factory new router, ZR1.
When running the test the DUT must take the role defined in its PICS. The other role must be played
by a golden unit or test harness.

Note: Need set Transiver volume to 0dB
Its set on MAC layer and need create global MAC function.
For added information see technical documentation on H/W.


ZED - ZLL ED, namely Dimmable light (single endpoint).
ZR  - ZLL router, namely Non-Color Scene Controller. The device is address-assignment-capable
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
