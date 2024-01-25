BL   : start address 0x08000000
FW1  : start address 0x08003000
FW2  : start address 0x08035800
       reserver address for FW2 0x8035700
NVRAM: start address 0x0802d400

read.bin  - fw without header and hash16
write.bin - fw with header and hash16

Please use utility create_image.exe for add header and hash16
create_image.exe read.bin write.bin