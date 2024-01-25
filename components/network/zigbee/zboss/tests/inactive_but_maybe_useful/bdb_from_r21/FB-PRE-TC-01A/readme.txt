5.1.1	FB-PRE-TC-01A: Device discovery - client side tests
This test verifies the generation of the device discovery request commands mandated by the BDB specification and handling of the respective responses, if defined.


Test procedure is:

- start DUT ZR - wait for Distributed net formation
- start ZR2, it joins DUT
- switch DUT off (kill process)
- start ZR1, it joins ZR2
- after join ZR1 starts F&B as Target
- start DUT again. It starts F&B as Initiator if already joined after nvram read
- wait for F&B between DUT and ZR1 done
- switch DUT off (kill process)
- start ZED, it joins ZR1
- after join ZED starts F&B as Target
- start DUT again. It starts F&B as Initiator if already joined after nvram read.
- wait for F&B between DUT and ZED done

Note: to satisfy test procedure, it is enough to just send approproate IEEE_addr_req.
