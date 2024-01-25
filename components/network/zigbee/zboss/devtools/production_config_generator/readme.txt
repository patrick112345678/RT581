Utility to generate production configuration block
============

This tool reads input such as desired channel masks, installation key etc from file
and generates binary block, which, if uploaded to device shall update Zigbee stack and
application configuration

=============
Help
 
To see available options and list of supported applications:
$ production_config_generator.exe -h

Currently supported applications:
- generic-device
- ias_zone_sensor
- smart_plug

See input_example.txt for input format list of parameters that canbe set in production configuration.

================
How to upload 
To write production configuration block to device, run following command
$ tcdb.exe wf 77000 -i output.bin -b -e.

Note that currently maximum production configuration size (stack+application+header) is 132 bytes

================
How to extend this tool.

On example of ias_zone_sensor:
1) Add new type to application_type_t enum and application_list array
  	- see application_type_e::IAS_ZONE_SENSOR enum
2) Include struct which corresponds with the one application expects to receive
  	- izs_production_config_t included along with izs_device.h header from IAS Zone application folder
3) implement function with type config_parameter_setter_t
   which, given pointer to application_production_config_t and key and value as string, would update config
	- see the typedef andset_ias_zone_section_parameter() function
4) Register new type in application_list array
5) in input type write new application type on the first line

