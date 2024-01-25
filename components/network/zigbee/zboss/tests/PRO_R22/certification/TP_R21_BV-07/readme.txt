TP/R21/BV-07 Rejecting Trust Center Rejoin with Well-known Link Key - ZC
Objective: To confirm that the ZC will reject a Trust Center rejoin attempt when that device has a well-known link key.  

Specification Requirements: 
•	The Trust Center shall not permit a device to perform a trust center rejoin using a well-known link key.

Initial conditions:
      DUT ZC
        |
       gZR
       
Test Procedure:
1. Disable permit join on DUT ZC.  This may involve rebooting DUT ZC so that it comes up in the default permit join state (disabled).
2. Through platform specific means, initiate a trust center rejoin on gZR.  This can also be done by having DUT ZC perform a NWK key update without notifying gZR.

Pass Verdict:
1. gZR shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
2. gZR sends a NWK Command, Rejoin request, without NWK encryption.  It shall use its previously obtained short address for the source field in the NWK header.
3. DUT ZC responds with a NWK Command, Rejoin Response, without NWK encryption.  The destination field in the NWK header shall be the previously obtained short address for gZR.  Rejoin Status shall be set to SUCCESS (0x00).
4. DUT ZC does not send gZR the NWK key.  It may optionally send a NWK leave message to gZR.

Fail Verdict:
1. gZR does not issue an MLME Beacon Request MAC command frame, or DUT ZC does not respond with a beacon.
2. gZR does not issue a NWK Command, rejoin request without NWK encryption.
3. DUT ZC does not issue a NWK command, Rejoin Response.  Or DUT ZC issues a Rejoin Response with NWK encryption.

