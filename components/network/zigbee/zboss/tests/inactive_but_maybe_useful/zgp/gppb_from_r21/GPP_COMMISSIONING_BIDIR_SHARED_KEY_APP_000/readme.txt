Test described in GP test specification, clause 5.4.1.2 Bidirectional commissioning, ApplicationID = 0b000; shared key

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-gps joins to network, th-gpd pairing with th-gps

5.4.1.2 Test procedure
commands sent:

1:
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter, ExitMode = On first Pairing success, Unicast communication = 0b0).
Commissioning action is (repeatedly, if required) performed on TH-GPD (if manually: not more often than once per second), so that TH-GPD sends GPD Channel Request command with:
    - MAC sequence number = N;
    - the sub-fields of the NWK Frame Control field set to:
       - Frame Type = 0b01,
       - Auto-Commissioning,
          - 0b0, if the GPD Channel Request transmission is immediately followed by reception window;
          - 0b1, if the GPD Channel Request transmission is immediately followed by another GPD Channel Request transmission, on the same or another channel.
    - and NWK Frame Control Extension = 0b0, and the Extended NWK Frame Control field absent;
    - the SrcID field and Endpoint field absent, the security not used (Security frame counter and MIC field absent);
    - preferably with Rx channel in the (second) next attempt indicating channel other than the operation channel of the DUT-GPP;
  until DUT-GPP sends GP Commissioning Notification with GPD Channel Request command and TH-GPS1 responds with GP Response with GPD Channel Configuration command
  and TransmitChannel != operational channel; in the GP Response, the ApplicationID sub-field is set to 0b000 and the GPD ID field carries 0x00000000.

2:
- Within the 5s TransmitChannel timeout, make TH-GPD send Channel Request GPDF on the TransmitChannel;
  with Frame Type sub-field of the NWK Frame Control field set to 0b01, Auto-Commissioning = 0b0, Frame Control Extension = 0b0;
  and the following fields absent: Extended NWK Frame Control, GPD SrcID, Endpoint, Security frame counter and MIC.

  Note: according to specofocation if GPP transmit GPDF to GPD then direction set to 0b1:
  A.1.4.1.3 Extended NWK Frame Control field
  The Direction sub-field SHALL be set to 0b0, if the GPDF is transmitted by the GPD, and to 0b1, if the GPDF is transmitted by a proxy.

3:
- 0.5s after TH-GPD Channel Request GPDF transmission on the TransmitChannel, make TH-GPD send Commissioning GPDF on the operational channel, with:
   - sub-fields of the NWK Frame Control field set to: Frame Type = 0b00, Auto-Commissioning = 0b0, Frame Control Extension = 0b1;
   - sub-fields of the Extended NWK Frame Control field set to: ApplicationID = 0b000, RxAfterTx = 0b1;
   - with GP Security Key request sub-field of the Options field set to 0b1
   - with sub-fields of the Extended Options field set to: GPD Key present = 0b1 and GPD Key encryption = 0b1,
   - the fields GPD Key and GPD Key MIC present.

4:
- Commissioning action is (repeatedly, if required) performed on TH-GPD (if manually: not more often than once per second),
  until TH-GPS1 provides success feedback, resulting in Commissioning GPDF as in step 3A.
- TH-GPS1 sends GP Response command with GPD Commissioning Reply command on receipt of each GP Commissioning Notification with GPD Commissioning command, carrying the shared key.

5:
- On receipt of GPD Commissioning Reply, TH-GPD sends correctly protected GPD Success command.

6:
- On receipt of GP Commissioning Notification with GPD Success command, TH-GPS1 provides user success feedback and sends GP Pairing.
- Read out Proxy Table entry of the DUT-GPP.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
