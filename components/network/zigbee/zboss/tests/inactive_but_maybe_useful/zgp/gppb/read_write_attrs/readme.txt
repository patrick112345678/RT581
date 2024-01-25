Test described in GP test specification, clause 5.1.2 GPP Attribute writing and reading

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC

dut_gpp starts as ZC, th-tool joins to network and start read/write attrs

5.1.2.2.1 Test procedure
commands sent by TH-tool:

5:
- read gppMaxProxyTableEntries attribute
- attempts to write a different value M to the gppMaxProxyTableEntries attribute
- reads out the value of the gppMaxProxyTableEntries attribute

6:
- read ProxyTable attribute
- atempts to write a different value - 2 bytes "0x0000" to the ProxyTable attribute
- reads out the value of the ProxyTable attribute

7:
- read gppFunctionality attribute
- attempts to wtite a different value M to the gppFunctionality attribute
- reads out the value of the gppFunctionality attribute

8:
- read gppActiveFunctionality attribute
- attempts to wtite a different value M to the gppActiveFunctionality attribute
- reads out the value of the gppActiveFunctionality attribute

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
