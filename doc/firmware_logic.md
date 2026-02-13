# Firmware Runtime Logic (High-Level)

[[_TOC_]]

## Overview

The controller firmware is built around a cooperative scheduler (`TaskScheduler`).
At runtime it is essentially:

- **One-time setup** in `setup()` → `inamata::setupNode(...)`.
- **A forever loop** that calls `scheduler.execute()`.
- **Tasks** and **controllers** that do the actual work:
  - **Connectivity task** keeps a network up and the WebSocket connected.
  - **System monitor task** periodically reports health metrics.
  - **Peripheral / Task / LAC / OTA / Behavior controllers** process server commands.

The server-facing interface is a **single WebSocket connection**. Incoming JSON
messages are fanned out to the different controllers, and outgoing messages are
serialized JSON objects (telemetry, results, register, system, etc.).

## Boot Process

### 1) MCU boot → Arduino `setup()`

Entry point: `src/main.cpp`.

- Calls `inamata::setupNode(services)`.
- If setup fails, waits 10 seconds and reboots.

### 2) `setupNode(...)` initializes subsystems

Implementation: `src/utils/setup_node.cpp`.

High-level sequence:

1. **Serial** starts (for logs).
2. **Storage** is created and filesystem opened.
3. **Secrets** are loaded (auth token, core domain, websocket path, WiFi AP list, etc.).
4. **Network services** are created:
   - `WiFiNetwork` is always created from stored WiFi AP credentials.
   - For GSM-enabled builds/devices (e.g. Fire Data Logger), `GsmNetwork` is also created.
5. **WebSocket** is created and configured with callbacks to:
   - `ActionController`
   - `BehaviorController`
   - `PeripheralController`
   - `TaskController`
   - `LacController`
   - `OtaUpdater`
6. **BLE server** is created (used for provisioning).
7. **Peripherals are loaded**:
   - **Fixed devices**: peripherals come from compiled fixed config.
   - **Dynamic devices**: peripherals are loaded from local storage.
   - If stored peripherals cannot be loaded, they are deleted to avoid boot loops.
8. Optional initialization (compile-time flags):
   - **RTC init**
   - **Configuration manager**
   - **Fixed-task startup** using stored behavior config
9. **System tasks are created** and enabled:
   - Connectivity
   - System monitor

At the end of `setupNode(...)`, the firmware is “running” in the sense that the
scheduler now has active tasks.

### 3) Arduino `loop()`

`loop()` just runs `scheduler.execute()`; all work happens in scheduled tasks.

## Connectivity & Provisioning

The connectivity task (`CheckConnectivity`) is the “main runtime orchestrator”.
It is responsible for:

- Choosing and bringing up a network (WiFi or GSM if available).
- Performing time sync (NTP) periodically.
- Maintaining the WebSocket connection.
- Falling back to provisioning mode on first boot / missing credentials.

### Provisioning mode

Provisioning is entered when:

- The WebSocket token (`ws_token`) is missing, or
- The device has **never successfully connected since boot** and the WebSocket
  connect attempt times out.

In provisioning mode the device:

- Enables BLE and runs **Improv (BLE)** provisioning.
- Accepts WiFi credentials, server URL overrides, and the controller auth token.
- Once provisioning finishes (or times out), it switches back to connect mode.

Notes:

- Some older / alternate build variants may also support a WiFi AP + captive
  portal workflow; see the project README and other docs for the user-facing
  setup steps.
- On GSM-capable devices, provisioning may intentionally omit a WiFi SSID; the
  firmware treats an empty SSID as “use GSM instead of WiFi”.

## Network Selection (WiFi vs GSM)

### WiFi-only devices

WiFi-only builds always use WiFi.

- `WiFiNetwork::connect()` runs as a non-blocking state machine:
  - Fast reconnect using cached SSID
  - Scan for known APs and connect to the strongest
  - Try hidden APs
  - Cycle modem power if needed

### GSM-enabled devices

On GSM-enabled devices, the connectivity task supports switching between WiFi
and GSM.

Current behavior (high-level):

- A hardware/firmware “toggle” input is sampled periodically.
- Based on that toggle, the firmware selects:
  - **WiFi mode**: WiFi radio on, GSM modem disabled.
  - **GSM mode**: WiFi radio off, GSM modem enabled, data connection brought up.

GSM connection management:

- The GSM modem is powered and initialized.
- The firmware periodically checks:
  - Network registration (“SMS / calls” capability)
  - GPRS/data connection
  - Signal quality and network system mode (EDGE/LTE/etc.)
- If registered but no data connection exists, it attempts `gprsConnect(GSM_APN)`.

## Time Synchronization

Accurate timestamps matter for telemetry and retry behavior.

- When connected, the firmware tries to synchronize time once every ~24 hours.
- On WiFi, it uses SNTP.
- On GSM, it uses the modem’s NTP/time facilities and updates system time.

Once time is synced, `Services::is_time_synced_` becomes true and outgoing
telemetry messages can include ISO timestamps.

## WebSocket Connection Lifecycle

The WebSocket is polled from the connectivity task.

On successful (re)connect:

- A **register** message is sent (device info, active peripherals/tasks, etc.).
- A **system** message may be sent with connection up/down durations.
- Buffered retry messages may start draining.

If the WebSocket cannot connect for too long:

- **Before the first successful connection after boot**: enter provisioning.
- **After the device has connected at least once**: keep retrying, and avoid
  provisioning during normal operation.

## Command Handling (Server → Device)

Incoming WebSocket JSON messages are deserialized and then passed to each
controller. Each controller looks only for “its” key(s) and ignores the rest.

### Controllers and what they handle

- **ActionController** (`action`):
  - Restart device
  - Clear stored resources
  - Factory reset
  - Identify (e.g. blink/beep via callback)

- **PeripheralController** (`peripheral`):
  - `sync`: replace stored peripherals and reboot
  - `add`: add peripheral and persist to storage
  - `remove`: remove peripheral and delete from storage

- **TaskController** (`task`):
  - `start`: start/restart a task (server-started tasks are tracked by UUID)
  - `stop`: stop a task (idempotent if not found)

- **LacController** (`lac`):
  - `start`: starts a Local Action Chain (a local task that runs sequences)

- **BehaviorController** (`behav`):
  - `set`: stores behavior config and applies it (callbacks)
  - `clear`: deletes stored behavior config

- **OtaUpdater** (`update`):
  - Downloads and applies an OTA firmware image.
  - Reports progress and status back as result messages.

### Results (Device → Server)

Most command handlers reply with a `type: "result"` message that includes:

- The `request_id` from the command (for tracing), when present.
- A per-command list of entries with `uuid`, `status`, and optional `detail`.

Additionally, when a **server-started task ends**, the TaskRemovalTask sends a
stop result back to the server (with any error that caused the task to stop).

## Message Sending (Device → Server)

The firmware sends a few message categories:

- **Register** (`reg`): sent on (re)connect to describe device state.
- **Telemetry** (`tel`): emitted by tasks/peripherals as they measure data.
- **Limit events** (`lim`): emitted when local threshold/limit logic triggers.
- **System** (`sys`): periodic health metrics and connection stats.
- **Errors / debug** (`err`, `dbg`): diagnostic messages.

### Telemetry packaging and timestamps

Telemetry typically includes:

- Initiator reference (task UUID and/or LAC UUID)
- Peripheral identity
- One or more data points `{ value, data_point_type }`
- Optional `time` (ISO timestamp)

If time is not synced yet, the firmware may send telemetry without a timestamp
and will generally avoid retry-buffering it.

### Retry buffering (best-effort)

If sending fails and the message includes a valid timestamp, the firmware can
buffer a small number of messages and retry after reconnect.

This is meant as a short outage buffer, not a durable offline store.

## Related Docs

- WebSocket message shapes: `doc/websocket_api.md`
- Flashing & bootstrapping: `doc/flash_and_boot.md`
- Peripheral configuration: `doc/peripherals.md`
