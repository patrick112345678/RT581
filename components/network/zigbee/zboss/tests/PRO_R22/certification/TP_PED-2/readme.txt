TP/PED-2 Child Table Management: End Device Initiator – ZED
Objective: DUT ZED sets end-device initiator bit in NWK header for all outgoing frames.

Initial Conditions:

     gZC
  |       |
  |       |
gZED   DUT ZED


Test Procedure:
1 Join device gZED to gZC.
2 Join device DUT ZED to gZC. (Criteria 1 – 6)
3 Observe DUT behaviour for at least one end-device keep-alive period (Criteria 7)
4 Let gZC send Buffer Test Request to DUT ZED (Criteria 7)
5 Let DUT ZED send Buffer Test Request to gZED (Criteria 8)


Pass Verdict:
1 DUT shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2 DUT is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3 DUT issues a ZDO device announcement sent to the broadcast address (0xFFFD), properly encrypted at the NWK,
  with the End Device Initiator bit in the NHR cleared.
4 DUT sends an End Device Timeout Request to gZC; with “Requested Timeout Enum” as any value between 0-14; and “Configuration” as 0.
5 In the End Device Timeout Response sent from gZC to DUT, the Status is equal to SUCCESS.
6 DUT sends a MAC-Data Poll to gZC, at least three times in every End Device Timeout duration and at
  least at the poll rate setup in the initial conditions, whichever is less. The MAC Data Poll is acknowledged
  by gZC, typically indicating no pending frame.
7 After having received the buffer test request from gZC, DUT sends Buffer Test Response to gZC with End Device Initiator
  bit in NHR set as TRUE
8 DUT sends Buffer Test Request to gZED with End Device Initiator bit in NHR set as TRUE.
  Note: gZC will forward the frame to gZED with initiator bit cleared. gZED will respond with initator bit set;
  gZC will forward the response to DUT ZED with initiator bit cleared.


Fail Verdict:
1 DUT does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2 DUT is not able to complete the MAC association sequence with gZC.
3 DUT does not issue a device announcement; or the device announcement is not properly encrypted;
  or the End Device Initiator bit in the NHR is set.
4 DUT does not send an End Device Timeout Request to gZC; or the “Requested Timeout Enum” is not in the range of 0-14;
  or “End Device Configuration” is non-zero.
5 In the End Device Timeout Response sent from gZC to DUT the “Status” is not SUCCESS and the status code indicates
  a malformed request.
6 DUT does not send a MAC-Data Poll to gZC, at least three times in every End Device Timeout duration or
  it violates the specified poll-rate as setup in the initial conditions.
7 DUT does not send Buffer Test Response to gZC; or the End Device Initiator bit in NHR set as FALSE in Buffer Test Response
8 DUT does not send Buffer Test Request to gZED; or the End Device Initiator bit in NHR is set as FALSE in Buffer Test Request

