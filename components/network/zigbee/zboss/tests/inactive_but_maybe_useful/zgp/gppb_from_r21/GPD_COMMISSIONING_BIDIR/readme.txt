Test described in GP test specification, clause 2.4.4.1	Bidirectional commissioning procedure

TH GPS            - GPS as ZC
DUT GPD           - GPD ON/OFF device

th_gps starts as ZC and enter in commissioning mode with exit mode:
gpsCommissioningExitMode = On first Pairing success

2.4.4.1 Test procedure
commands sent by DUT:

1:
A:
- TH-GPS enters commissioning mode with gpsCommissioningExitMode = On first Pairing success.
- Commissioning action is (repeatedly, if required) performed on DUT-GPD (if manually: not more often than once per second),
  until TH-GPS provides commissioning success feedback.
B:
- Upon transmission opportunity following reception of a correctly formatted GPD Channel Request command  with Auto-Commissioning = 0b0,
  the TH-GPS sends correctly formatted GPD Channel Configuration command with:
    - the sub-fields of the NWK Frame Control field set to: FrameType = 0b01 and NWK Frame Control Extension = 0b0,
    - and the Extended NWK Frame Control field absent;
    - the fields GPD SrcID, Endpoint, Security frame counter and MIC are absent.
C:
- Upon transmission opportunity following reception of a ceorrectly formatted GPD Commissioning command  with RxAfterTx = 0b1,
  the TH-GPS sends correctly formatted GPD Commissioning Reply command with:
    - the sub-fields of the NWK Frame Control field set to: Frame Type = 0b00 and NWK Frame Control Extension = 0b1,
    - the sub-fields of the Extended NWK Frame Control field set to: ApplicationID as in the triggering Commissioning GPDF; RxAfterTx = 0b0, Direction = 0b1;
    - the GPD ID fields corresponding to the ApplicationID,
    - command payload carrying information as requested by the DUT-GPD.
D:
- After commissioning completes, make DUT-GPD send Data GPDF.

2:
A: GPD with decommissioning/reset function (GPCF10A: YES):
- Make sure TH-GPS exited commissioning mode.
- DUT-GPD is made to send a Data GPDF.
- Reset DUT-GPD in DUT-specific way.
- Commissioning action is performed on DUT-GPD.
- If required, commissioning action is performed again on DUT-GPD.
B: GPD without decommissioning/reset function (GPCF10A: NO):
- Perform commissioning between TH-GPS and DUT-GPD.
- Make sure TH-GPS exited commissioning mode.
- Do NOT reset/decommission the DUT-GPD.
- Commissioning action is performed on DUT-GPD.
- If required, commissioning action is performed again on DUT-GPD.
C: GPD with decommissioning/reset function (GPCF10A: YES):
- Make sure TH-GPS exited commissioning mode.
- DUT-GPD is made to send a Data GPDF.
- Do NOT reset/decommission the DUT-GPD.
- Commissioning action is performed on DUT-GPD.
- If required, commissioning action is performed again on DUT-GPD.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
