Test described in GP test specification, clause 2.4.4.3 Bidirectional commissioning with PANId request

TH GPS            - GPS as ZC
DUT GPD           - GPD ON/OFF device

th_gps starts as ZC and enter in commissioning mode with exit mode:
gpsCommissioningExitMode = On first Pairing success

2.4.4.3 Test procedure
commands sent by DUT:

1:
- TH-GPS enters commissioning mode with gpsCommissioningExitMode = On first Pairing success.
- Commissioning action is (repeatedly, if required) performed on DUT-GPD (if manually: not more often than once per second),
  until TH-GPS provides commissioning success feedback, TH-GPS correctly responds to all DUT-GPD requests.

2:
- Trigger DUT-GPD to send Data GPDF.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
