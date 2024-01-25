ZBOSS IAS Zone Sensor application (IZS)

============================================================================
IZS application implements Zigbee 3.0 specification and Base device behavior (end device, ED). Generic IAS Zone device
implementation is suitable for reuse while any IAS Zone device implementation (smoke detector, door open/close, etc.)
IZS application may be tested with simple_gw ZC.

IZS includes the following ZCL clusters
Mandatory for all the roles:
1) Basic (s)
2) Identify (s)
3) IAS Zone (s)
4) OTA FW upgrade (c)
Following clusters are supported for battery powered ED:
1) Poll control (s)
2) Power Configuration (s)

============================================================================
Startup:
On starting up, IZS application does association attempt if a device was not associated to a network yet
or it starts rejoining, if it was joined to a network before.
IZS application implements a set of states that may be used to indicate device status change:
- Association or rejoining is in progress
- Association is finished (success or fail)
- Rejoining is finished
- etc
These states may be used to provide LED indication or some other kind of user indication.

============================================================================
Production configuration data:
Production configuration data (PCD) may be used to specify a device parameters that may be changed in production
time to produce different flavors of a device. The following parameters must be specified as a part of PCD:
3) Manufacturer name (will be copied to Basic cluster attribute)
4) Model Id (will be copied to Basic cluster attribute)
PCD is read and applied on application start time (before ZDO is started).
For tools and readmes on generating PCD refer to production_config_generator

============================================================================
ZB association:
IZS joins a target network while 16 other open networks are running (ZB3.0, HA or SE). Background: if a network is
not suitable for some reason, it will be blacklisted and its beacons will be filtered out during association. 
If association was unsuccessful, IZS sleeps for (N * 5) seconds and then continue association attempts. N starts with 1
and enlarges with each attempt.

============================================================================
Rejoining:
IZS supports rejoin back-off procedure. On detecting a communication link failure with its parent, IZS starts rejoin
back-off procedure.

============================================================================
Reset to factory defaults:
IZS supports reset to FD in the following cases:
1) Device leaves a network
2) Receives a Reset to FD command
3) Action is initiated by a user. How to trigger a reset to FD by user action is out of scope for IZS.

============================================================================
Enrollment procedure:
1) Support Auto-pair scenario
2) Support Trip-to-pair scenario

============================================================================
Notifications sending:
Event queueing algorithm for notification sending is supported.

============================================================================
Power configuration cluster:
1) Attributes for 1 battery pack are supported.
2) Supported battery threshold: 60 days.
3) ZoneStatus attribute will be updated if battery defect or low battery is detected.

============================================================================
OTA FW upgrade cluster:
IZS supports ZCL OTA upgrade.

============================================================================
Fast polling:
Fast polling is independent of the poll-control cluster short and/or long poll interval.
Existing ZBOSS adaptive poling is enabled.

============================================================================
Detector support:
Detector control API (called by the main app):
1) Detector HW init
2) Detector HW enable
3) Detector HW disable

API function izs_update_ias_zone_status() supports the status change events:
1) Alarm status set/clear
2) Temper status set/clear
3) Defect status set/clear
4) Test mode status set/clear
5) Low battery stats set/clear
6) Battery defect set/clear
7) Battery 60 days threshold set/clear
This function may be called by the detector hw handler module.
