Sniffer application based on NRF52840 platform. To be used with ZBOSS Sniffer application and Wireshark.
DSR zigbee air sniffer software is available for download from:
http://zboss.dsr-wireless.com/projects/zboss/wiki/ZBOSS_Sniffer
Wireshark is available for download from:
https://www.wireshark.org/

Application configures radio in promiscuous mode and send captured packets via USB-UART to host.
Sniffer application support several commands from host: stop/start and select channel.

Setup:
===========
Flash nRF52840-Preview-DK board with the sniffer.

How to use:
Run ZBOSS Sniffer application at host:
 - select sniffer COM port (Device field)
 - specify path to Wireshark or path to pcap file
 - select channel
 - remove Diagnostick info in FCS check mark
 - start session
 - also you can pause/resume sniffer session

Note: 
1) sniffer session can be stopped/restarted directly from Wireshark.
2) You can change channel while sniffer paused and resume capture without restarting session.
3) Also it is possible to use one sniffer with several devices and to get packets from several channels in one Wireshark window.
   The information about channel number can be included in every packet (just enable diagnostic info in sniffer window).
