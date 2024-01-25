Test described in GP test specification, clause 4.1.2 GPP Attribute writing and reading

Test Harness tool - as ZR
DUT GPCB          - GPCB as ZC

dut_gpcb starts as ZC, th-tool joins to network and start read/write attrs

4.1.2.2.1 Test procedure
commands sent by TH-tool:

1:
- read gpsCommunicationMode attribute
- attempts to write a different value M to the gpsCommunicationMode attribute
- reads out the value of the gpsCommunicationMode attribute

2:
- read gpsCommissioningExitMode attribute
- attempts to write a different value M to the gpsCommissioningExitMode attribute
- reads out the value of the gpsCommissioningExitMode attribute

3:
- read gpsCommissioningWindow attribute
- attempts to write a different value M to the gpsCommissioningWindow attribute
- reads out the value of the gpsCommissioningWindow attribute

4:
- read gpsSecurityLevel attribute
- attempts to write a different value M to the gpsSecurityLevel attribute
- reads out the value of the gpsSecurityLevel attribute

5:
- read gpsMaxSinkTableEntries attribute
- attempts to write a different value M to the gpsMaxSinkTableEntries attribute
- reads out the value of the gpsMaxSinkTableEntries attribute

6:
- read Sink Table attribute
- attempts to write a different value M to the Sink Table attribute
- reads out the value of the Sink Table attribute

7:
- read gpsFunctionality attribute
- attempts to write a different value M to the gpsFunctionality attribute
- reads out the value of the gpsFunctionality attribute

8:
- read gpsActiveFunctionality attribute
- attempts tp write a different value M to the gpsActiveFunctionality attribute
- reads out the value of the gpsActiveFunctionality attribute

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
