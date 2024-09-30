[[_TOC_]]

# Boot Sequence

## ESP32

On power on, the ESP32 runs its ROM based first stage bootloader. This hands over to the 2nd stage bootloader (at <span dir="">0x1000</span>). It uses the partition table (at <span dir="">0x8000</span>) to find the otadata partition. The data in this partition is used to select which app (firmware) slot to execute (0 or 1). Using the partition table, it finds the entry point to the app slot and hands over execution to it. The app is the compiled and packaged Arduino-based program.

## ESP8266

On power on, the ESP8266 runs its ROM based first stage bootloader. This hands over to the 2nd stage bootloader that was compiled into the firmware image. It uses the partition table (also compiled into the image) to find the data regarding which app (firmware) slot to execute (0 or 1). Using that information, it hands over execution to it.

## Differences Between the ESP32 and ESP8266 Platforms

For the ESP8266, the 2nd stage bootloader and partition tables are compiled into the firmware image. In the ESP32's case, they are flashed separately. This means that the Flasher has to download the partition table and bootloader for the ESP32 in addition to its firmware, where as for the ESP8266 only the firmware is required. This means however, that supporting different flash sizes and layouts is easier for the ESP32, since only the partition table has to be customized instead of compiling a new image. This has the effect, that a larger number of ESP8266 firmware lines have to be maintained as this complexity is stored in them. This is especially true as different ESP8266 boards need different bootloaders depending on the flash interface (DIO, QIO, QOUT, DOUT).

# File System

LittleFS (LFS) is the file system used by both platforms. During the flash process, secrets as well as locally synced peripherals, tasks and LACs are stored there.

The TLS CA certificates are stored in the firmware as bundle for ESP32-based devices. As ESP8266 are not on the roadmap, they do not verify TLS CA certificates.

The version of the current LFS library and disk are defined by the included [esp_littlefs](https://github.com/joltwallet/esp_littlefs) library. To determine the current versions, match

- `ESP_LITTLEFS_VERSION_NUMBER` in `framework-arduinoespressif32/tools/sdk/esp32/include/esp_littlefs/include/esp_littlefs.h`
- with the git-submodule include in `esp_littlefs/src/littlefs` of the GitHub repository.
- The `LFS_VERSION` and `LFS_DISK_VERSION` defines are located in that version of `lfs.h` in the [littlefs GitHub porject](https://github.com/littlefs-project/littlefs).

At the time of writing the constants were:

- ESP_LITTLEFS_VERSION_NUMBER: "1.5.0"
- LFS_VERSION: "0x00020005" = 2.5
- LFS_DISK_VERSION: "0x00020000" = 2.0

These versions are relevant in the [Inamata Flasher](https://github.com/InamataIO/Flasher/) that generates a LittleFS partition with the secrets. The disk version that is generated may not be newer than the disk version supported by the firmware.

## secrets.json

The secrets configuration is stored on the flash in `secrets.json` for both ESP32 and ESP8266 variants. Below is an example of the secrets JSON.

```
{
  "wifi_aps": [
    { "ssid": "Mindspace2.4", "password": "HelloThere" },
    { "ssid": "HasenbauOben", "password": "" },
    { "ssid": "Berge24", "password": "EinPasswort" }
  ],
  "ws_token": "d60f9b8fb0474649b584fa897e76394f272bdc36",
  "core_domain": "core.inamata.io",
  "secure_url": true,
  "name": "Controller 3"
}
```

# On-Device Encryption

The [Espressif docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html) give a comprehensive overview of flash encryption. This should be used in combination with secure boot which ensures that only signed binaries are executed.

When using flash encryption in production mode, it is only possible to update the firmware via OTA updates, USB flashing is disabled. The encryption itself is transparent to reading/writing the data from the firmware itself, which means that it doesn't require any code changes. Additional material to help enabling it are listed below:

- https://community.platformio.org/t/protect-arduino-sketch-on-esp32-via-flash-encryption/12665/5
- https://github.com/pedros89/ESP32-update-both-OTA-Firmware-and-OTA-SPIFFS-with-Arduino