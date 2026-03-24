# Improv BLE Command Reference

This document describes all Improv BLE commands implemented by the controller firmware, including the `X_*` command extensions defined in [lib/improv-sdk/improv.h](/home/moritz/Documents/Inamata/Software/Controller/lib/improv-sdk/improv.h).

The behavior documented here reflects the current firmware implementation in [src/managers/ble_improv.cpp](/home/moritz/Documents/Inamata/Software/Controller/src/managers/ble_improv.cpp) and the parser/response helpers in [lib/improv-sdk/improv.cpp](/home/moritz/Documents/Inamata/Software/Controller/lib/improv-sdk/improv.cpp).

## BLE characteristics

- Service UUID: `00467768-6228-2272-4663-277478268000`
- Status characteristic: `00467768-6228-2272-4663-277478268001`
- Error characteristic: `00467768-6228-2272-4663-277478268002`
- RPC command characteristic: `00467768-6228-2272-4663-277478268003`
- RPC result characteristic: `00467768-6228-2272-4663-277478268004`
- Capabilities characteristic: `00467768-6228-2272-4663-277478268005`

Requests are written to the RPC command characteristic. Successful command responses are notified on the RPC result characteristic. Errors are typically notified separately on the error characteristic.

## RPC frame format

The firmware consumes and produces Improv RPC frames in the following format:

```text
+--------+-------------+-------------------+----------+
| Byte 0 | Byte 1      | Bytes 2..n-2      | Byte n-1 |
+--------+-------------+-------------------+----------+
| CMD    | data_length | length-prefixed   | checksum |
|        |             | payload fields    |          |
+--------+-------------+-------------------+----------+
```

Notes:

- `checksum` is the low 8 bits of the sum of all prior bytes in the frame.
- Response payload items are encoded as repeated `len + bytes` string fields.
- For commands without parameters, the request is typically just `CMD`, `0x00`, `checksum`.
- For commands that use two parameters, the parser maps them to `ssid` and `password` fields in `improv::ImprovCommand`.

## Error reporting

Most command failures do not generate an RPC result payload. Instead, the firmware notifies the error characteristic with one of these values from [lib/improv-sdk/improv.h](/home/moritz/Documents/Inamata/Software/Controller/lib/improv-sdk/improv.h):

| Error                      | Value  | Meaning in this firmware                                                          |
| -------------------------- | ------ | --------------------------------------------------------------------------------- |
| `ERROR_NONE`               | `0x00` | No error                                                                          |
| `ERROR_INVALID_RPC`        | `0x01` | Bad checksum, malformed payload, or invalid mobile-operator list                  |
| `ERROR_UNKNOWN_RPC`        | `0x02` | Unsupported or unrecognized command                                               |
| `ERROR_UNABLE_TO_CONNECT`  | `0x03` | Used by WiFi provisioning, not by the `X_*` commands below                        |
| `ERROR_NOT_AUTHORIZED`     | `0x04` | Command requires `STATE_AUTHORIZED`                                               |
| `X_ERROR_ALREADY_SCANNING` | `0xFE` | A WiFi or mobile network scan request was received while a scan is already active |
| `ERROR_UNKNOWN`            | `0xFF` | Generic internal failure, for example JSON handling or storage failure            |

## Command summary

| Command                          | Opcode | Request parameters                                       | Success response                                                                            |
| -------------------------------- | ------ | -------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `UNKNOWN`                        | `0x00` | Not a client command; parser fallback on malformed input | No RPC result; handled as unknown RPC error                                                 |
| `WIFI_SETTINGS`                  | `0x01` | Two strings: SSID, password                              | Async response `WIFI_SETTINGS` with one URL string after provisioning succeeds              |
| `IDENTIFY`                       | `0x02` | None                                                     | Empty RPC result for same opcode; device identify action                                    |
| `GET_CURRENT_STATE`              | `0x02` | None                                                     | Alias of `IDENTIFY` in current enum/dispatch                                                |
| `GET_DEVICE_INFO`                | `0x03` | None                                                     | Five strings: firmware name, firmware version, board+MAC, device type name, controller name |
| `GET_WIFI_NETWORKS`              | `0x04` | None                                                     | One RPC result per AP, then one empty completion response                                   |
| `X_SET_ALLOWED_MOBILE_OPERATORS` | `0xF9` | Optional one string: comma-separated MCC/MNC list; empty clears allowlist | Empty RPC result for the same opcode                                                        |
| `X_GET_MOBILE_OPERATORS`         | `0xFA` | Optional one-string scan mode: `auto`, `gsm`, or `lte`   | One RPC result per operator tuple, then one empty completion response                       |
| `X_GET_MOBILE_STATE`             | `0xFB` | None                                                     | Six strings: ICCID, IMEI, connection_state, operator_mcc_mnc, signal_quality, nsm           |
| `X_SET_USER_DATA`                | `0xFC` | One string chunk containing JSON text                    | Empty RPC result for the same opcode                                                        |
| `X_GET_DEVICE_TYPE`              | `0xFD` | None                                                     | One string: device type id                                                                  |
| `X_SET_SERVER_AUTH`              | `0xFE` | Two strings: URL, auth token                             | Empty RPC result for the same opcode                                                        |
| `BAD_CHECKSUM`                   | `0xFF` | Not a client command; internal parser sentinel           | No RPC result; handled as invalid RPC error                                                 |

## `WIFI_SETTINGS` (`0x01`)

Starts provisioning using WiFi credentials.

### Request payload

This command is parsed as two strings:

1. `ssid`
2. `password`

### Normal response

The provisioning flow is asynchronous:

- If `WiFi.begin(...)` starts successfully, state becomes `STATE_PROVISIONING`.
- Once the device is connected, the firmware sends:

```text
command = 0x01
payload = [core_domain]
```

### Special behavior

- If SSID is empty, the firmware assumes non-WiFi provisioning (for example GSM path), sets `STATE_PROVISIONED`, and triggers identify.

### Error behavior

- If current state is not `STATE_AUTHORIZED`: error characteristic notifies `ERROR_NOT_AUTHORIZED`.
- If `WiFi.begin(...)` fails immediately: error characteristic notifies `ERROR_UNABLE_TO_CONNECT`.
- If connection times out later in provisioning: error characteristic notifies `ERROR_UNABLE_TO_CONNECT` and state returns to `STATE_AUTHORIZED`.

## `IDENTIFY` (`0x02`)

Triggers the device identify action.

### Request payload

- none

### Normal response

The firmware sends an empty RPC result for the same opcode:

```text
command = 0x02
payload = []
```

The identify action is triggered as a side effect.

### Error behavior

No command-specific BLE error is emitted by this handler.

## `GET_CURRENT_STATE` (`0x02`)

`GET_CURRENT_STATE` shares opcode `0x02` with `IDENTIFY` in the current enum.

Practical effect in current firmware:

- Dispatch lands in the `IDENTIFY` handler path.
- The command behaves as `IDENTIFY`, not as a dedicated state query.

## `GET_DEVICE_INFO` (`0x03`)

Returns static and runtime identity strings.

### Request payload

- none

### Normal response

The firmware returns five strings in this order:

1. Firmware name (from `FIRMWARE_VERSION` prefix)
2. Firmware version (from `FIRMWARE_VERSION` suffix)
3. Board name with MAC suffix (`<board>@<mac>`)
4. Device type name (`Storage::device_type_name_`)
5. Controller name

```text
command = 0x03
payload = [fw_name, fw_version, board_and_mac, device_type_name, controller_name]
```

### Error behavior

No command-specific BLE error is emitted by this handler. If Improv is stopped, no response is sent.

## `GET_WIFI_NETWORKS` (`0x04`)

Starts an async WiFi scan and streams results.

### Request payload

- none

### Normal response

When scan results are available, the firmware sends one RPC result per access point:

```text
command = 0x04
payload = [ssid, rssi, "YES"|"NO"]
```

After all AP entries, it sends an empty completion response:

```text
command = 0x04
payload = []
```

### Error behavior

- Already scanning: error characteristic notifies `X_ERROR_ALREADY_SCANNING` (`0xFE`), no RPC result is sent, and the current scan continues.
- Internal scan failure (`scanComplete() < -1`): scan is aborted without an explicit BLE error or RPC error frame.

## `X_SET_ALLOWED_MOBILE_OPERATORS` (`0xF9`)

Stores the list of mobile network operators the modem is allowed to use.

### Request payload

The parser treats this command as a single-field command:

- `ssid`: optional operator list string

If `ssid` is non-empty, it must be a comma-separated list of MCC/MNC identifiers, for example:

```text
65501,65510
```

Validation rules enforced by the handler:

- Empty payload (`ssid` length `0`) is valid and clears the configured allowlist.
- Each item must contain only digits.
- Each item must be 5 or 6 digits long.
- Empty items within a non-empty list are rejected.

### Normal response

On success, the device sends an RPC result with the same opcode and no payload fields:

```text
command = 0xF9
payload = []
```

### Error behavior

- Invalid list format: error characteristic notifies `ERROR_INVALID_RPC`, no RPC result is sent.
- Failure while storing the list: error characteristic notifies `ERROR_UNKNOWN`, no RPC result is sent.

## `X_GET_MOBILE_OPERATORS` (`0xFA`)

Starts a modem network scan and streams discovered operator tuples back over BLE.

### Request payload

The request may include one optional scan-type string:

- `auto`
- `gsm`
- `lte`

The parser stores this value in `command.ssid`, and the handler maps it to the modem scan mode.

If the parameter is omitted (empty payload) or does not match one of the supported strings, the firmware falls back to the default scan mode.

### Normal response

This command can generate multiple RPC result notifications.

For each top-level tuple returned by the modem `+COPS` scan result, the firmware sends:

```text
command = 0xFA
payload = [operator_tuple]
```

Example tuple payload:

```text
2,"CHN-UNICOM","UNICOM","46001",7
```

After all tuples are sent, the firmware sends a final empty response to mark completion:

```text
command = 0xFA
payload = []
```

### Error behavior

- Already scanning: error characteristic notifies `X_ERROR_ALREADY_SCANNING` (`0xFE`), no RPC result is sent, and the current scan continues.
- Scan failure or timeout: firmware sends one normal RPC result `payload = [cops_result]` (often empty) and does not update the error characteristic.

## `X_GET_MOBILE_STATE` (`0xFB`)

Returns modem identity and connectivity state values.

### Request payload

- none

### Normal response

The firmware returns six strings in this order:

1. SIM ICCID
2. Modem IMEI
3. `connection_state` bitfield:
   - bit 0: attached to a mobile network operator
   - bit 1: IP data link established (GPRS connected)
4. Operator MCC/MNC tuple (5-6 digits)
5. Signal quality (`CSQ`): `0..31`, or `99` for unknown/undetectable
6. Network system mode (`NSM`):
   - `0`: no service
   - `1`: GSM
   - `2`: GPRS
   - `3`: EGPRS (EDGE)
   - `4`: WCDMA
   - `5`: HSDPA only (WCDMA)
   - `6`: HSUPA only (WCDMA)
   - `7`: HSPA (HSDPA and HSUPA, WCDMA)
   - `8`: LTE

```text
command = 0xFB
payload = [iccid, imei, connection_state, operator_mcc_mnc, signal_quality, nsm]
```

### Error behavior

There is no command-specific error handling in `BleImprov`. If GSM support is compiled in and the service is active, the handler directly queries the modem and sends the response.

## `X_SET_USER_DATA` (`0xFC`)

Passes custom JSON data to registered user-data handlers in the BLE server.

### Request payload

The parser treats this command as a single-field command:

- `ssid`: one chunk of JSON text

The handler appends each chunk to an internal `user_data_` string. It keeps accumulating chunks while the current chunk length is `>= 200`. Once a chunk shorter than 200 bytes arrives, it treats the accumulated text as a complete JSON document and parses it.

Effective protocol:

- zero or more continuation chunks of length `>= 200`
- one final chunk of length `< 200`

### Normal response

On success, the device sends an RPC result with the same opcode and no payload fields:

```text
command = 0xFC
payload = []
```

Successful behavior is:

- accumulated JSON is parsed
- each registered user-data handler is called with the parsed JSON object
- RPC result is emitted as confirmation

### Error behavior

- JSON parse failure: error characteristic notifies `ERROR_UNKNOWN`, no RPC result is sent.
- Any user-data handler returning `false`: error characteristic notifies `ERROR_UNKNOWN`, no RPC result is sent.

## `X_GET_DEVICE_TYPE` (`0xFD`)

Returns the firmware device type identifier.

### Request payload

- none

### Normal response

The firmware returns one string:

1. `Storage::device_type_id_`

```text
command = 0xFD
payload = [device_type_id]
```

### Error behavior

There is no command-specific error response. If the Improv service is stopped, the handler returns without sending a response.

## `X_SET_SERVER_AUTH` (`0xFE`)

Stores the controller WebSocket endpoint and authentication token.

### Request payload

The parser treats this as a two-field command:

1. `ssid`: WebSocket URL string
2. `password`: auth token

Observed handler semantics:

- If the URL string is non-empty, the firmware parses it with `yuarel`.
- `ws://` is treated as insecure, everything else is treated as secure.
- The host and path are stored via the `WebSocket` and `Storage` services.
- If the URL string is empty, the stored URL is cleared.
- The auth token is always stored, even when the URL is empty.
- The handler clears locally stored peripherals before applying the new configuration.

### Authorization requirement

This command requires the Improv state to be `STATE_AUTHORIZED`.

If the state is not authorized, the error characteristic notifies:

```text
ERROR_NOT_AUTHORIZED (0x04)
```

### Normal response

On success, the device sends an empty RPC result with the same opcode:

```text
command = 0xFE
payload = []
```

### Error behavior

- Not authorized: error characteristic notifies `ERROR_NOT_AUTHORIZED`, no RPC result is sent.
- Invalid URL format in non-empty URL string: error characteristic notifies `ERROR_INVALID_RPC`, no RPC result is sent.

## `UNKNOWN` (`0x00`) and `BAD_CHECKSUM` (`0xFF`)

These are special internal values in the command enum.

### `UNKNOWN` (`0x00`)

Produced by the parser for malformed frames (for example invalid field lengths or length mismatch). It is not expected as a normal client command.

Runtime behavior:

- Falls through the default switch case.
- Error characteristic notifies `ERROR_UNKNOWN_RPC`.

### `BAD_CHECKSUM` (`0xFF`)

Produced by the parser when checksum verification fails.

Runtime behavior:

- Hits the explicit `BAD_CHECKSUM` switch case.
- Error characteristic notifies `ERROR_INVALID_RPC`.

## Implementation references

- Command enum definitions: [lib/improv-sdk/improv.h](/home/moritz/Documents/Inamata/Software/Controller/lib/improv-sdk/improv.h)
- Request parsing and response encoding: [lib/improv-sdk/improv.cpp](/home/moritz/Documents/Inamata/Software/Controller/lib/improv-sdk/improv.cpp)
- BLE command dispatch and command handlers: [src/managers/ble_improv.cpp](/home/moritz/Documents/Inamata/Software/Controller/src/managers/ble_improv.cpp)
- Handler comments and state fields: [src/managers/ble_improv.h](/home/moritz/Documents/Inamata/Software/Controller/src/managers/ble_improv.h)
