Test described in GP test specification, clause 4.1.2.2	Sink Attributes

Test Harness tool - as ZR
Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network

4.1.2.2 Test procedure
commands sent by TH-tool and TH-gpd:

1:
- TH-ZR reads out the value of the gpsCommunicationMode attribute, using ZCL Read Attributes command.
- TH-ZR writes a different value M (e.g. byte 0x02) to the gpsCommunicationMode attribute, using ZCL Write Attributes No Response command.
- TH-ZR reads out the value of the gpsCommunicationMode attribute.

2:
- TH-ZR reads out the value of the gpsCommissioningExitMode attribute.
- TH-ZR writes a different value M (e.g. byte 0x00) to the gpsCommissioningExitMode attribute.
- TH-ZR reads out the value of the gpsCommissioningExitMode attribute.

3:
- TH-ZR reads out the value of the gpsCommissioningWindow attribute.
- TH-ZR writes different value M (e.g. 2 bytes 0x003d) to the gpsCommissioningWindow attribute.
- TH-ZR reads out the value of the gpsCommissioningWindow attribute.

4:
- TH-ZR reads out the value of the gpsSecurityLevel attribute.
- TH-ZR writes a byte different value M (e.g. 0x02) to the gpsSecurityLevel attribute.
- TH-ZR reads out the value of the gpsSecurityLevel attribute.

5:
- TH-ZR reads out the value of the gpsMaxSinkTableEntries attribute.
- TH-ZR attempts to write a different value M (e.g. byte 0x0f) to the gpsMaxSinkTableEntries attribute, using ZCL Write Attributes No Response command.
- TH-ZR reads out the value of the gpsMaxSinkTableEntries attribute.

6:
- TH-ZR reads out the value of the Sink Table attribute.
- TH-ZR attempts to write a different value M (e.g. two bytes 0x0000) to the Sink Table attribute, using ZCL Write Attributes No Response command.
- TH-ZR reads out the value of the Sink Table attribute.

7:
- TH-ZR reads out the value of the gpsFunctionality attribute.
- TH-ZR attempts to write a different value M (e.g. two bytes 0xffff) to the gpsFunctionality attribute, using ZCL Write Attributes No Response command.
- TH-ZR reads out the value of the gpsFunctionality attribute.

8:
- TH-ZR reads out the value of the gpsActiveFunctionality attribute.
- TH-ZR attempts to write a different value M (e.g. 0x0000) to the gpsActiveFunctionality attribute, using ZCL Write Attributes No Response command.
- TH-ZR reads out the value of the gpsActiveFunctionality attribute.

9: Negative test: empty Sink Table:
A:
- Clean the Sink Table. 
- TH unicasts GP Sink Table Request with Index = 0x00.
B:
- Establish a pairing between DUT-GPS and TH-GPD identified by GPD SrcID N (ApplicationID = 0b000), using single-/multi-hop pairing, if DUT-GPS supports it and security if DUT-GPS supports it.
- TH-ZR reads the Sink Table attribute.
C:
- Keep the pairing as created in step 9. 
- TH unicasts GP Sink Table Request with Index = 0x00.

10:
- Keep the Sink Table entry from step 9A.
- Establish a pairing between DUT-GPS and TH-GPD identified by GPD IEEE address N  and Endpoint X (ApplicationID = 0b010), using single-/multi-hop pairing, if DUT-GPS supports it and security if DUT-GPS supports it.
- TH-ZR reads the Sink Table attribute.

11:
- Keep the pairing as created in step 9.
- Create additional pairing between TH-GPD2 and TH-GPS, including security, for GPD ID = M.
- TH unicasts GP Sink Table Request with Index = 0x01.

12:
A:
- Keep the pairing as created in step 9 and 11.
- TH unicasts GP Sink Table Request with SrcID = N.

B:
- Keep the pairing as created in step 9 and 11.
- TH unicasts GP Sink Table Request with GPD IEEE address = N and Endpoint X.

13: Negative test: wrong Index:
- Keep the pairing as created in step 9 and 11.
- TH unicasts GP Sink Table Request with Index = 0x03.

14: Negative test: wrong GPDID:
A:
- Keep the pairing as created in step 9 and 11.
- TH unicasts GP Sink Table Request with SrcID other than N and M.
B:
- Keep the pairing as created in step 9 and 11.
- TH unicasts GP Sink Table Request with GPD IEEE address other than N.

15: Opt. Negative test: if DUT-GPS does NOT implement a Proxy Table:
- TH unicasts GP Proxy Table Request with Index = 0x00.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
