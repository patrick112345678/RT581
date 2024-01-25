47.4.2.2 Subcase: Decommissioning GPDF and GP Pairing with RemoveGPD = 0b1
Initial conditions:
1 The Proxy Table attribute of DUT-GPP has an entry with GPD ID N, for which the
EntryActive and EntryValid subfields of the Options field have shall both be
set to 0b1.

Test Procedure:
1) Make DUT-GPP enter commissioning mode.
   Make TH-GPD send appropriately secured Decommissioning GPDF.
   If required, make DUT-GPP exit commissioning mode.
2) If item 1 succeeds, this follows automatically: (Make) TH-GPT send(s) a
GP Pairing command with GPD ID value N and the Remove GPD sub-field of the
Options field having value 0b1.
Read the Proxy Table attribute and the gppBlockedGPDID attribute of DUT-GPP.

Expected Outcome1
Pass verdict:
1) DUT-GPP sends GP Commissioning Notification with Security processing
failed = 0b0, carrying the GPD Decommissioning command (0xE1), and indicating
the command is payloadless.
2) After DUT-GPP receives the GP Pairing with RemoveGPD = 0b1, and the DUT-GPP:
supports the Proxy Table Maintenance feature, then
EITHER the Proxy Table of DUT-GPP contains an entry with GPD ID value N, the
value of the sub-fields EntryActive, EntryValid, LighweightUnicast GPS,
Derived Group GPS, Commissioned Group GPS of the Options parameter are equal
to 0b0, 0b1, 0b0, 0b0, 0b0, respectively, and the Sink address list,
Sink Group list and Search Counter parameters are absent.
OR
the gppBlockedGPDID attribute of the DUT-GPP contains an entry for the GPD ID.
the DUT-GPP does NOT support the Proxy Table Maintenance feature,
then the Proxy Table of DUT-GPP does NOT contain any entry for TH-GPD GPD ID N.

Fail Verdict:
1) DUT-GPP does not send GP Commissioning Notification AND/OR the GP
Commissioning Notification is not formatted as described in
47.4.2.2.3/pass verdict 1 above.
2) If DUT-GPP supports Proxy Table Maintenance feature, neither Proxy Table
nor gppBlockedGPDID attribute of DUT-GPP contains an entry with GPD ID value N,
OR
Proxy Table does contain such an entry with is NOT formatted exactly as defined
in 47.4.2.2.3/pass verdict 2 above.
If DUT-GPP supports Proxy Table Maintenance feature, Proxy Table of DUT-GPP
still contains an entry for TH-GPD GPD ID N.
