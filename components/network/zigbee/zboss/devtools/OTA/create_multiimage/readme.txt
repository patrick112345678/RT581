Utility for creating multiimage OTA files

Usage: {file_version} {manufacturer_code} {image_type} {radio app version} {sensor app version} {'rl78only'|'rfonly'|'cfgonly'}
Example: create_image.exe v02040201 0x1133 0x0401 0x1234 0x5678

Separate binary files with the following names should reside in directory with the executable:
- config_block
- sensor_firmware
- radio_firmware

If file isn't present in the directory, warning will be printed out and corresponding section will be omitted in the
image.

Special logic for config block.
OTA File version is given as the first parameter to this utility.
If configuration section file is present, last byte of configuration section version overwrites the second byte of file version.

