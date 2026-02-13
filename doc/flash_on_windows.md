# Flash on Windows

1. Open a Git Bash terminal
2. Source Python's virtual env: `source /c/Users/tommi/.platformio/penv/Scripts/activate`
3. Flash the device:

```
python C:/Users/tommi/.platformio/packages/tool-esptoolpy/esptool --port COM5 --chip esp32s3 --baud 921600 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size detect 0x0000 C:/Users/tommi/Documents/Inamata/FDL/bootloader.bin 0x8000 C:/Users/tommi/Documents/Inamata/FDL/partitions.bin 0xe000 C:/Users/tommi/Documents/Inamata/FDL/boot_app0.bin 0x10000 C:/Users/tommi/Documents/Inamata/FDL/ima_fire_data_logger@1.18.1.bin
```
