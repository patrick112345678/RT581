Test checks point-to-point inter-PAN exchange as described in ZLL inter-PAN
exchange LLD.

intrp_zc starts as ZC, intrp_zed joins to it, then intrp_zed sends commands as
described in Test Procedure (see ALL inter-PAN exchange test LLD). After
packet sent and response received, devices stop communication.

To start test in Linux with network simulator:
- run ./run.sh
- view .pcap file by Wireshark
