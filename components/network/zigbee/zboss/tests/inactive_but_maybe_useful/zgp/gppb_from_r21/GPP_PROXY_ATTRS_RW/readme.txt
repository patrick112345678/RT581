Test described in GP test specification, clause 5.1.2.2 GPP Attribute writing and reading

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC

dut_gpp starts as ZC, th-tool joins to network and start read/write attrs

5.1.2.2 Test procedure
commands sent by TH-tool:

5:
- TH-tool reads out the value of the gppMaxProxyTableEntries attribute, using ZCL Read Attributes command.
- TH-tool attempts to write a different value M (e.g. byte 0x0a) to the gppMaxProxyTableEntries attribute, using ZCL Write Attributes No Response command.
- TH-tool reads out the value of the gppMaxProxyTableEntries attribute.

6:
- TH-tool reads out the value of the Proxy Table attribute, using ZCL Read Attributes command.
- TH-ZR attempts to write a different value - 2 bytes 0x0000 to the Proxy Table attribute, using ZCL Write Attributes No Response command.
- TH-tool reads out the value of the Proxy Table attribute.

7:
- TH-tool reads out the value of the gppFunctionality attribute, using ZCL Read Attributes command.
- TH-ZR attempts to write a different value (e.g. two bytes 0xffff) to the gppFunctionality attribute, using ZCL Write Attributes No Response command.
- TH-tool reads out the value of the gppFunctionality attribute.

8:
- TH-tool reads out the value of the gppActiveFunctionality attribute, using ZCL Read Attributes command.
- TH-ZR attempts to write a different value (e.g. 0x0000) to the gppActiveFunctionality attribute, using ZCL Write Attributes No Response command.
- TH-tool reads out the value of the gppFunctionality attribute.

9:
A: Empty Proxy Table: 
- TH unicasts GP Proxy Table Request with Index = 0x00.
B:
- Send GP Pairing command for TH-GPS with all settings as supported by DUT-GPP, including security, for SrcID = N (ApplicationID = 0b000).
- TH-ZR reads the Proxy Table attribute of DUT-GPP, using ZCL Read Attributes command.
C:
- Keep the pairing as created in step 9B.
- Send GP Pairing command for TH-GPS with all settings as supported by DUT-GPP, including security, for GPD IEEE address = N (ApplicationID = 0b010) and Endpoint = X.
- TH-ZR reads the Proxy Table attribute of DUT-GPP, using ZCL Read Attributes command.

10:
A:
- Keep the pairing(s) as created in step 9. 
- TH unicasts GP Proxy Table Request with Index = 0x00.
B:
- Keep the pairing(s) as created in step 9.
- TH unicasts GP Proxy Table Request with Index = 0x01.

11:
- Keep the pairing(s) as created in step 9.
- Send GP Pairing command for TH-GPS with all settings as supported by DUT-GPP, including security, for SrcID = M (ApplicationID = 0b000).
- TH unicasts GP Proxy Table Request with  Index = 0x02.

12:
A:
- Keep the pairing(s) as created in step 9 and 11.
- TH unicasts to DUT-GPP the GP Proxy Table Request with GPD SrcID = N (ApplicationID = 0b000).
B:
- Keep the pairing(s) as created in step 9 and 11.
- TH unicasts to DUT-GPP the GP Proxy Table Request with GPD IEEE address = N (ApplicationID = 0b010) and Endpoint = X.

13: Negative test: wrong Index:  
- Keep the pairing(s) as created in step 9 and 11.
- TH unicasts GP Proxy Table Request with Index = 0x03.

14:
A: Negative test: wrong GPDID:
- Keep the pairing(s) as created in step 9 and 11.
- TH unicasts to DUT-GPP the GP Proxy Table Request with GPD SrcID other than N and M (ApplicationID = 0b000).
B: Negative test: wrong GPDID:
- Keep the pairing(s) as created in step 9 and 11.
- TH unicasts to DUT-GPP the GP Proxy Table Request with GPD IEEE address = N (ApplicationID = 0b010) and Endpoint other than X.

15: Opt. Negative test: if DUT-GPP does NOT implement a Sink Table: 
- TH unicasts GP Sink Table Request with Index = 0x00.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
