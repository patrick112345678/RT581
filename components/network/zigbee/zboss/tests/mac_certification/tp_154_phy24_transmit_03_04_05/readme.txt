Common test for TP/154/PHY24/TRANSMIT-03/04/05

Sends 100 packets on every channel, then pause for 30 sec to
reconfigure Signal Analyzer


Test TP/154/PHY24/TRANSMIT-03

Tests that the transmit center frequency tolerance for each channel of the DUT is within a maximum
offset from the specified center frequency as given in the table below.

Transmit Center Frequency                Transmit Center Frequency
Maximum Offset                           Offset for 2.4 GHz Band

+/- 40ppm                                +/- 96.2 kHz at 2405 MHz
                                         to
                                         +/- 99.2 kHz at 2480 MHz
                                         
Test Procedure:
1. DUT shall transmit a MAC compliant PHY
Payload (PSDU) 41 88 xx AA 1A FF FF 44
33 01 23 45 67 89 AB CD EF FE DC BA 98
76 54 32 10 CRC1 CRC2 where xx is the
mac sequence number which can be any value
and the FCS (CRC1 and CRC2) is computed
normally. The signal level shall be -3 dBm or
higher. If the DUT is capable of properly
transmitting at a maximum output power
greater than -3 dBm then the output power of
the DUT shall be set to this maximum output
power level.
Measure, over a 3 MHz bandwidth, the
frequency of the peak spectral value of the
DUT for the channel being tested.

2. Calculate the deviation of the measured peak
value from the ideal peak value at the Center
frequency for the channel being tested.
Repeat Measurement and Calculation and
record for 10 samples.

3.Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 
Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 


Pass: For each sample in each channel in the
band of operation the deviation of the
transmission frequency of the DUT is within
+/- 40ppm.
Fail: For any sample of any channel in the
band of operation the deviation of the
transmission frequency of the DUT is not
within +/- 40ppm.

Test TP/154/PHY24/TRANSMIT-04

Tests that the DUT is capable of transmitting a standards conforming signal and packet with an
arbitrary payload and correct CRC at least the minimum specified power level, given in the table
below, for each channel.

Minimum Specified Tx Power -3 dBm
                                         
Test Procedure:
1. DUT shall transmit a standards conforming
signal and packet with an arbitrary bit pattern
for the payload (PSDU), and correct CRC,
where the payload consists of at least 20
bytes. The output power of the DUT shall be
set to the maximum output power level.
Set the signal/spectrum analyzer to trigger on
the detected RF/IF power.
If a power meter with averaging detector is
used, correct the measurement results by the
stated duty cycle.

2.Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 
Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 

Pass: For each channel in the band of operation
the DUT is capable of transmitting at least
-3dBm output power.

Fail: For any of the channels in the band of
operation the DUT is not capable of
transmitting at least -3dBm output power.

Test TP/154/PHY24/TRANSMIT-05

Tests that the DUT transmits signals at maximum operating power according to the power spectral
482 density mask limits specified in the table below for each channel, to ensure no leakage to adjacent
483 channels, and sufficient signal quality for the band of operation.

Frequency Offset          Relative Limit      Absolute Limi
|f-fc| > 3.5 MHz             -20 dB              -30 dBm

Test Procedure:
1.DUT shall transmit a standards conforming
signal and packet with an arbitrary bit pattern
for the payload (PSDU), and correct CRC,
where the payload consists of at least 20
bytes. The output power of the DUT shall be
set to the maximum output power level.
Set the signal/spectrum analyzer to trigger on
the detected RF/IF power.
Using the settings of signal/spectrum
analyzer, detect the highest power level
within the channel being measured.

2. Identify the maximum power values for:
frequencies < (center frequency - Frequency
Offset) down to 2400 MHz
and
frequencies > (center frequency + Frequency
Offset) up to 2483.5 MHz
to determine the absolute power levels.

3. Using the delta marker functionality
determine the relative power levels by first
noting the peak. For relative limit test note
the peak value of the PSD in the range
(center frequency + 1000 kHz and center
frequency - 1000 kHz) as Power Ref.

4.Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 
Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 

Pass:
1. For frequency offsets values greater
than Frequency Offset all the PSD
values from the absolute power
spectral density tests for all* channels
in the band of operation are less than
-30 dBm.
* In the case of Channel 26 this test
may be performed at a Tx Power
Level less than the maximum output
power level of the device, provided
that the Tx Power Level used is at
least -3dBm. In the case where the
maximum output power level of the
device is not used in the testing of
Channel 26, the Tx Power Level used
to successfully pass the -30dBm test is
to be recorded as part of the Pass/Fail
test results.
2. For frequency offsets values greater
than Frequency Offset all the PSD
values from the relative power spectral
density tests for all channels in the
band of operation are more than 20 dB
below the Power Ref. value for the
corresponding channel.


Fail:
1. For frequency offsets values greater
than Frequency Offset any PSD value
from the absolute power spectral
density test for any** channel in the
band of operation is more than -30
dBm.
** See exception above in Pass
Criteria for testing on Channel 26.
2. For frequency offsets values greater
than Frequency Offset any PSD value
from the relative power spectral
density test for any channels in the
band of operation is less than 20 dB
below the Power Ref. value for the
corresponding channel.
