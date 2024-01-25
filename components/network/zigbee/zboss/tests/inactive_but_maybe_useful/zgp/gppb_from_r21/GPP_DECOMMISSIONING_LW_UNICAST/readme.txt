5.4.2.5 Subcase: Lightweight unicast pairing removal with GP Pairing with AddSink = 0b0

Initial Conditions: The Proxy Table attribute of DUT-GPP has an entry with GPD
ID N for lightweight unicast communication mode, for which the EntryActive and
EntryValid sub-fields of the Options field are both set to 0b1; and with Lightweight
sink address list containing two GPS with short addresses: 0xMMMM and 0xNNNN.

Test procedure:
1) TH-GPS sends GP Pairing command with the sub-fields of the Options field set
to AddSink = 0b0, RemoveGPD = 0b0, CommunicationMode = 0b11 and the address 0xMMMM
in the Lightweight sink address list.
Read the Proxy Table attribute and the gppBlockedGPDID attribute of DUT-GPP.

2) TH-GPS sends GP Pairing command with the sub-fields of the Options field set
to AddSink = 0b0, RemoveGPD = 0b0, CommunicationMode = 0b11 and the address
0xNNNN in the Lightweight sink address list.
Read the Proxy Table attribute and the gppBlockedGPDID attribute of DUT-GPP.

Expected Outcome:
Pass verdict:
1) After DUT-GPP receives the GP Pairing with AddSink = 0b0, the Proxy Table of
DUT-GPP contains an entry with GPD ID value N, the value of the sub-fields
EntryActive = 0b1, EntryValid 0b1, LightweightUnicastGPS = 0b1, Lightweight
sink address list containing only one GPS with short address: 0xNNNN.
2) If DUT-GPP supports Proxy Table maintenance, it sets the entry to inactive
valid or shifts the GPD to gppBlockedGPDID list, with SearchCounter=0x00;
does not support Proxy Table maintenance, it removes the entry (or adds it to gppBlockedGPDID).
If DUT-GPP does not support Proxy Table maintenance, it removes the entry (or adds it to gppBlockedGPDID).


Fail verdict:
1) DUT-GPP does not remove the 0xMMMM sink AND/OR DUT-GPP removes/sets to inactive/moves to gppBlockedGPDID  the entry for GPD ID = N.
2) DUT-GPP does not act as described in 47.4.2.4.3/item 2 above.
