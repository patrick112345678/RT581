OTA Upgrade image create utility
================================

This utility creates OTA Upgrade image file for device
upgrading over the Zigbee network

Usage
================================

1.  Prepare binary with application code.
2.  Generate OTA image from application firmware using create_image.exe utility, for example:
    create_image.exe ota_client.bin ota_client.ota v01.02.01.01 0xdead 0x0012
where file version == 0x01020101, manufacturer code == 0xdead, image_type == 0x0012.
3.  The utility will create output file specified in command parameters (in example above this is ota_client.ota),
which contains OTA Upgrade header and hash16 and is ready to be used for device upgrading.

OTA file format: OTA Header| app tag (2 bytes)| app length (4 bytes)| hash tag (2 bytes)| hash length (4 bytes)| hash16 (16 bytes)
where hash16 is AES-MMO hash function based on AES128.
For more details see "Zigbee Document 095264r23" (Zigbee Over-the-Air Upgrading Cluster),
see 6.3 - OTA File Format and 6.3.10.1 - Hash Value Calculation.

From 6.3.10.1:
4) "The hash value shall be calculated using the Matyas-Meyer-Oseas cryptographic hash
specified in section B.6 of [R5 - Zigbee Specification]. The hash is calculated starting with the OTA image header
and spanning just before the hash sub-element header, i.e. the calculation takes in to account
the first byte of the image header up to the last byte of the sub-element preceding the hash
sub-element."
