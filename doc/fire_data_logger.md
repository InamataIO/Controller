# Fire Data Logger (FDL)

## Scope and audience

This document describes the **externally observable behavior** of the Fire Data Logger
(FDL) firmware variant, with a focus on **alarms** and how they turn into notifications.

## What the device does (high-level)

The FDL monitors a fire suppression system using **digital inputs** and reports events to
Inamata:

- It detects alarm conditions (e.g. fire alarms, pump faults, prolonged runtimes).
- It sends **limit events** to the server, which drive push/app notifications.
- In cellular mode, it can additionally send **SMS alerts**.
- It **always logs input transitions locally** for regulatory/audit purposes.

## Fixed tasks (FDL build)

The FDL firmware variant starts a small set of **fixed tasks** (see `src/tasks/fixed/fire_data_logger/`).
These tasks are what make the device behave like an alarm logger rather than a generic controller.

High-level responsibilities:

- **`NetworkState`**: Drives the status LEDs to indicate provisioning/connectivity state and signal strength.
- **`Alarms`**: Evaluates alarm/limit conditions and emits limit events; handles maintenance mode suppression.
- **`Telemetry`**: Reports input state to the server (primarily on change, with periodic snapshots).
- **`LogInputs`**: Writes an on-device audit trail of input transitions and performs log housekeeping.
- **`Heartbeat`**: Blinks the heartbeat LED to show the firmware loop is alive.

## Alarms: from input to notification

### Inputs and triggering model

FDL inputs trigger when the input is connected to **common ground (COM)**:

- Closing input-to-COM → logical high (active)
- Opening input-to-COM → logical low (inactive)

The firmware assumes a fixed input naming/mapping so the same wiring produces consistent alarm semantics.

### Limit events (server-side notifications)

Every time an alarm condition becomes active or clears, the device emits a **limit event**
to the Inamata server.

These events notify users via the configured notification groups and severity thresholds.

Conceptually:

- **Start**: alarm becomes active
- **End**: alarm clears
- (Optional) **Continue/reminders**: used to re-notify for alarms that remain active

### Alarm types (high-level)

The FDL supports multiple classes of alarm/limit logic. The exact set of limits and their
thresholds are configured from the server (behavior/limits configuration), but the intent is:

- **Boolean (digital) alarms**: an input going active triggers an alarm (and clearing it ends the alarm).
- **Duration / runtime alarms**: an input staying active longer than a configured time triggers an alarm.
  This is used for “running too long” style conditions.
- **Activation-rate alarms**: too many activations (rising edges) within a time window triggers an alarm.
  This is used for “cycling too often” style conditions.

In all cases:

- The device emits limit events to the server on start/end (and may emit periodic continues while active).
- While **maintenance mode** is active, alerting is suppressed (no limit events and no SMS).

### Notification channels

The main notification route is app/browser push notifications. SMS is an additional channel:

- **Push/app/web notifications**: driven by server-side notification configuration.
- **SMS**: sent by the device in **cellular mode** when configured with SMS contacts.

Important: maintenance mode suppresses alerting (see below).

## Maintenance mode (alert suppression)

Maintenance mode exists to prevent nuisance alarms while service work is performed.

- Can be activated by pressing the on-board maintenance button or using the maintenance input.
- Status LED turns **red**.
- The **maintenance relay (relay 1)** closes.
- While active, input events are still logged locally, but alarms are **not sent** to the server
  and **no SMS alerts** are sent.
- Automatically exits **one hour** after activation.

## LEDs and operator-visible states

This is the primary “at a glance” debugging surface during installation and service.

### Power / heartbeat

- **Power LED (red)**: on when power is supplied.
- **Heartbeat LED**: blinks at a consistent 0.5s interval when the main loop is running.
  If it is off/on (not blinking), the program loop is blocked.

### GSM status LED (cellular modem)

- **Off**: modem is off
- **On**: attempting to connect to cellular network
- **Slow blink**: non-data connection established
- **Fast blink**: data connection established

### FDL status RGB LED (device state + signal strength)

| Color  | Meaning                                                        |
| ------ | -------------------------------------------------------------- |
| Green  | Provisioning/setup mode (ready to be configured via Bluetooth) |
| Purple | No network signal / not connected                              |
| Blue   | Connected (Wi-Fi or cellular)                                  |
| Red    | Maintenance mode override (alerts suppressed)                  |

Signal strength is shown via a repeating blink pattern:

- 1 blink = poor (may drop with environmental changes)
- 2–4 blinks = progressively stronger

## Connectivity and provisioning behavior

### Wi-Fi vs cellular selection

The device can connect via:

- **2.4 GHz Wi‑Fi**, or
- **cellular (3G/LTE)** using a built-in eSIM (roaming capable)

- Wi‑Fi-only testing/configuration can be done over USB power.
- For cellular mode, using **12–24 VDC** is strongly recommended (USB-only cellular can be degraded).
- First cellular registration can take **15–30 minutes**, especially in low-signal environments.

### Provisioning mode

The device enters provisioning mode (green status LED) when it has not been configured yet.

- If the device has connected to the server since boot, it will not re-enter provisioning mode
  just because connectivity later drops.

Ways to force provisioning mode:

- Remove all network antennas and power cycle, or
- Perform a factory reset via the USB serial terminal menu.

Provisioning requires BLE and a compatible setup client:

- Android app, or
- Chromium-based browser (Chrome/Edge/Opera)

## Local logs and timekeeping

Local logging is mandatory for regulatory/audit reasons:

- Input events are always stored locally and can be accessed via USB.
- Timestamps come from a battery-backed RTC.

- If the device cannot obtain the current time (e.g. RTC battery dead and no internet), events are
  still logged but start counting from **January 1st, 2000**.
- The device deletes the oldest logs once the total exceeds **10,000** events.

- If the RTC battery is depleted, “Battery dead” is shown in the serial menu.
- The RTC battery bridges cumulative power loss up to ~2 years (only time without external power counts).

## USB serial terminal menu (service interface)

- Connect via USB and press `c` to enter the menu.
- Serial: `115200` baud, `8N1`.
- Terminal settings: CRLF for receiving new lines, CR for enter.

Key functions:

- Configure SMS contacts
- Set location name (used for SMS text)
- Print logs
- View/set system time
- Factory reset (menu `R`, then type `reset`)

## Firmware integration notes (developer-facing, high-level)

From the firmware perspective, the FDL is a fixed-IO device that continuously:

- samples the digital inputs,
- generates telemetry and limit events,
- applies alarm/limit configuration delivered from the server,
- manages alert suppression during maintenance mode,
- and keeps local audit logs.

On the wire, it uses the standard controller WebSocket protocol described in:

- [WebSocket API](websocket_api.md)
- [Firmware Runtime Logic](firmware_logic.md)
