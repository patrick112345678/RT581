Test TP/154/PHY24/TRANSMIT-01

Tests that the DUT supports transmission of the correct modulation, spreading and bits per symbol for 
the band of operation at the maximum operating power. 

Under a strong signal condition (-3 dBm or higher output power level) a standards conforming signal
and packet with a specified payload and correct CRC shall be sent by the DUT. The packet shall be a 
MAC compliant frame with a PHY Payload (PSDU) of “41 88 xx AA 1A FF FF 44 33 01 23 45 67 89 
AB CD EF FE DC BA 98 76 54 32 10 CRC1 CRC2”, where xx is the mac sequence number which can 
be any value and the FCS (CRC1 and CRC2) is computed normally.

The corresponding data sequence is demodulated with the Signal Analyzer to verify proper modulation,
spreading and bits per symbol. 


Test Procedure:
1. DUT shall transmit a MAC compliant PHY 
Payload (PSDU) of “41 88 xx AA 1A FF FF 
44 33 01 23 45 67 89 AB CD EF FE DC BA 
98 76 54 32 10 CRC1 CRC2” where xx is 
the mac sequence number which can be any 
value and the FCS (CRC1 and CRC2) is 
computed normally. The signal level shall be 
-3 dBm or higher. If the DUT is capable of 
properly transmitting at a maximum output 
power greater than -3 dBm then the output 
power of the DUT shall be set to this 
maximum output power level. 
Signal Analyzer shall receive and detect the 
exact same chip pattern transmitted by the 
DUT for 100 packets. 

Pass: For each channel in the band of operation 
none of the bits, symbols or chips of the 
transmitted packets are received in error.

Fail: For any channel in the band of operation 1
or more of the bits, symbols or chips of the
transmitted packets are received in error. 

2.Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 
Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 


Test Description:

dut.bin is a Device Under Test(DUT).
DUT sends 100 packets per channel with 0.1 seconds delay.
DUT switches to the next channel automatically after whole packets sent for the given channel.