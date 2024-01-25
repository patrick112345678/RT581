THE MOST IMPORTANT NOTE: ALWAYS USE WIN_COM_DUMP FROM TRUNK!!!

win_com_dump (lin_com_dump) utility used to collect trace and dump from the device.
It is able to parse logs come from both JTAG or COM port.
To start working copy it to the root stack path.
Now you can try to run it:

win_com_dump [or lin_com_dump] {source_flag} {path} {log output name} {dump output name}

After that log output will be in human readable format.
Use dump_converter to do so with dump output.

Examples:

COM port source:
win_com_dump.exe -I \\.\COM4 trace.log traf.dump

JTAG source:
win_com_dump.exe -J trace.swo trace.log traf.dump


Put traf.dump to the Linux box which has Wireshark with our plugin installed.
Convert traf.dump to wireshark format using dump_converter utility:

Unix:
./dump_converter -ns traf.dump traf.pcap
Windows:
dump_converter.exe traf.dump traf.pcap

View .pcap by wireshark:
wireshark traf.pcap
