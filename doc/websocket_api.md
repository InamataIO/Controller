# WebSocket API

## Controller Messages

### Command

```
{
  type: "cmd",
  request_id: "...",
  peripheral: {
    sync: [{ uuid: "", ... }],
    add: [{ uuid: "", ... }],
    remove: [{ uuid: "" }]
  },
  task: {
    start: [{ uuid: "", ... } ],
    stop: [{ uuid: "" }],
    status: True
  },
  update: {
    url: "",
    size: int
  },
  lac: {
    start: { uuid: "", "routine": []},
    stop: { uuid: ""},
    install: { uuid: "", "routine": []},
    uninstall: { uuid: ""}
  },
  behavior: {
    set: { ... }
  }
}
```

The server translates `run_until` parameters for task start commands to `duration_ms` parameters. This is due to lacking datetime arithmetic on the controllers and the need to be able to restart tasks on errors. Therefore, sending the server `duration_ms` will result in an error.

### Telemetry

```
{
  type: "tel",
  task: "...",
  peripheral: "...",
  <time: "...",>
  data_points: [
    {
      <time: "...",>
      value: 0-9,
      data_point_type: "..."
    }
  ]
}
```

### Limit Event

Events can be start (str), continue (con) and end (end). If time is not
specified, the server sets the current time. The fixed peripheral ID (fp_id)
and fixed data point type ID (fdpt_id) are mapped to peripherals and DPTs of
that organization on the server. The controller limit ID (limit_id) is set as
a response to the register message.

```
{
  type: "lim",
  <time: "...",>
  value: float,
  event: <"str", "con", "end">,
  limit_id: UUID/str,
  fp_id: UUID/str,
  fdpt_id: UUID/str
}
```

### Results

```
{
  type: "result",
  <request_id: str>
  peripheral: {
    <add, remove>: [
      {
        uuid: UUID/str,
        status: <"success", "fail">,
        <detail: str>
      }
    ],
  }
  task: {
    <start, stop>: [
      {
        uuid: UUID/str,
        status: <"success", "fail">,
        <detail: str>
      }
    ],
  },
  lac: {
    <install, uninstall, start, stop>: [
      {
        uuid: UUID/str,
        status: <"success", "fail">,
        <detail: str>
        state: <"none", "running", "install_run", "install_end", "install_fail">
        version: int
      }
    ]
  }
  update: {
    status: <"fail", "updating", "finish", "success">,
    detail: str
  }
}
```

### Register

Versioned peripherals will send the PVS (Peripheral VersionS) list with the
peripheral ID and version. The lvs is the respective version list for LACs.

For fixed devices, the behavior ID (bid), behavior version (bv) and fixed
peripherals (fps) are sent instead. These list the fixed ID (fid) of the
peripherals and their fixed data point types (fdpts). The device field is a
well-known controller type.

```
{
  type: "reg",
  <peripherals: [UUID/str, ...]>,
  <pvs: [{id: UUID/str, v: int}, ...]>,
  <tasks: [UUID/str, ...]>,
  <lacs: [{uuid: UUID/str, state: <lac_state>, version: int, <detail: str>}]>,
  <lvs: [{id: UUID/str, v: int}, ...]>,
  <
  device: UUID/str,
  bid: UUID/str,
  bv: int,
  fps: [{fid: UUID/str, fdpts: [{fid: UUID/str, <prefix: str>}]}],
  >
}
```

### System

```
{
  type: "sys",
  ...
}
```

## Tasks

### Start: `tasks/<uuid>/start`

The start command starts a new task. Each start command has to have the following JSON parameters in addition to peripheral specific parameters.

| parameter  | content                       |
| ---------- | ----------------------------- |
| type       | type of the peripheral        |
| peripheral | unique name of the peripheral |

On successful creation of the task, the following JSON is returned. In order to stop a long running task, its ID has to be stored on creation and then sent when it is to be stopped. The _type_ corresponds to the task's type while the _peripheral_ equals the name of the peripheral being used by the task. This may also be null.

| parameter  | content                           |
| ---------- | --------------------------------- |
| id         | unique ID of the task             |
| type       | type of the task                  |
| peripheral | name of the peripheral being used |

### Stop: `tasks/<uuid>/stop`

In order to stop a task, send its ID to the stop topic.

| parameter | content               |
| --------- | --------------------- |
| id        | unique ID of the task |
