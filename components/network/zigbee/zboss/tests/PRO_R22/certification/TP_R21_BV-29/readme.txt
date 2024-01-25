TP/R21/BV-29 Alias usage – Device_annce reception
Objective: Verify that the DUT ZR/ZC/ZED is capable of changing it's short address if Network conflict 
concerned with the green power devices. Network address of gZC also substed with Alias usage.

Initial Conditions:

gZC and DUT-ZR operate in the same ZigBee PAN.

Test Set-Up:
    The DUT-ZR shall be in wireless communication proximity of gZC.
    A packet sniffer shall be observing the communication over the air interface.

    DUT-ZR
      |
      |
     gZC

The required network set-up for the test harness is as follows:

DUT-ZR
    PANid= Generated in a random manner (within the range 0x0001 to 0x3FFF)
    Short address = 0xWXYZ. Noted with sniffer. NV store settings and remove power.
    IEEE address=manufacturer-specific

gZC
    Pan id=same PANID as TH-GPT
    Logical Address=Generated in a random manner (within the range 0x0000 to 0xFFF7, but must be restarted in the case  it is equal to the alias address)
    IEEE address=manufacturer-specific
Note: a TH-tool capable of sending the required GP-related commands (Device_annce) is preferably used instead of the full TH-GPP

Test Procedure:
1  [R5] A.2.6
  Make gZC send Device_annce with Alias_short_addr WXYZ 
  (conflicting with short address of DUT-ZR) and IEEE address 0xFFFFFFFFFFFFFFFF.

Pass verdict:
1 On reception of Device_annce, DUT-ZR changes its NWK short address as specified in ZigBee PRO
    (and announces that via Device_annce).

Fail verdict:
1 On reception of GP Notification, DUT-ZR does not change its NWK short address as specified in ZigBee PRO.
    AND/OR the DUT-ZR does not announce that via Device_annce.

Additional info:
To start test use ./runng.sh 
