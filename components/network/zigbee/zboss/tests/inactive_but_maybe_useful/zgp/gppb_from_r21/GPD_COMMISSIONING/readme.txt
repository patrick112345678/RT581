Test described in GP test specification, clause 2.4.2 Base Commissioning GPDF frame format

TH GPS            - GPS as ZC
DUT GPD           - GPD ON/OFF device

th_gps starts as ZC and enter in commissioning mode with exit mode:
gpsCommissioningExitMode = On first Pairing success

2.3.4 Test procedure
commands sent by DUT:

1:
TH-GPS enters commissioning mode with gpsCommissioningExitMode = On first Pairing success.
DUT-GPD sends a Commissioning GPDF.

2:
Incrementing frame counter: 
Put the TH-GPS in commissioning mode.
DUT-GPD sends a Commissioning GPDF.
Put the TH-GPS back in operational mode (if it doesn't happen automatically).

3:
Incrementing frame counter:
DUT-GPD sends a Data GPDF.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
