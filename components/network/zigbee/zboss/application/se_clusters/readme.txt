This directory contains samples of next devices with zigBee3 clusters from SmartEnergy:
* Emetteur Radio Local (ERL) device sample
* In-home Display device sample

# se_clusters/erl_interface
* ERL Device - Working as zigBee Coordinator (ZC)
zigBee ERL Linky Meter Interface (ERL Interface) device behavior for operation on a Zigbee-PRO network.
This sample shows the basic implementation of functionality in the Energy Monitoring and Control application domains.
The individual device specifications will become part of the approved device specifications supported by the Zigbee Alliance.
It is based on and conforms to Zigbee-PRO, the Base Device Behavior and the Zigbee Cluster Library.
#Supported Clusters:
Server Role 
  * 0x0000 : Basic
  * 0x0003 : Identify 
  * 0x000A : Time
  * 0x0b01 : Meter Identification
  * 0x0b04 : Electrical Measurement
  * 0x0b05 : Diagnostics
  * 0x0700 : Price
  * 0x0702 : Metering
  * 0x0703 : Messaging
  * 0x0707 : Calendar
Client Role
  * 0x0003 : Identify
  * 0x000A : Time

# se_clusters/in_home_display
* In-Home Display Device - zigBee Router role (ZR)
This device is a client for some of the clusters, which the ERL Devices incorporates as Server clusters.
It is intended for connecting with ERL Meter Interface and send some commands and handle some replies from ERL.
#Supported Clusters:
Server Role 
  * 0x0000 : Basic
  * 0x0003 : Identify 
Client Role
  * 0x0707 : Calendar
  * 0x0702 : Metering
  * 0x0700 : Price
  * 0x0703 : Messaging
  * 0x0003 : Identify

## Operation

* ERL Devices acts as ZigBee Coordinator it accepts connection from other devices and have some server clusters on it, can reply to several cluster commands and Read Attributes commands.

* In-Home Display acts as ZigBee Router. After successfull join/restart already joined it starts BDB Finding&Binding procedure (as initiator)
and find any device with complimentary server role clusters.
After 180 seconds In-Home Display starts BDB Finding&Binding as target device.
ERL Device starts BDB Finding&Binding as target on boot and after 180 seconds starts BDB Finding&Binding as initiator.

* After Finding&Binding ERL device should send PublishCalendar command to In-Home Display automatically.

#IHD periodically send Get Current Price command and other commands,
#ERL answers with Publish Price and other cluster commands.
