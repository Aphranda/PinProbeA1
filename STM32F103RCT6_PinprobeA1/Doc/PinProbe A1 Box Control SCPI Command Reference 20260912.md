## PinProbe A1 Box Control SCPI Command Reference

> Document updated: `2026-09-12`. This document is intended for customer-side host integration, device control, and maintenance reference. The control-mode functions have been implemented and verified in bench and real-device integration tests.

---

## Device Information

|Item|Description|
|--|--|
|Manufacturer|GTS|
|Model|PINPROBEA1|
|Serial Number|Read from current SCPI IDN configuration|
|Firmware Version|Read from current SCPI IDN configuration|

## Basic Commands (IEEE 488.2 Mandated Commands)

|Command|Parameter|Description|
|--|--|--|
|`*CLS`||Clear status and error queue|
|`*IDN?`||Query device identification|
|`*RST`||Reset SCPI parser state|
|`*STB?`||Query status byte|
|`*WAI`||Wait for command completion|
|`*OPC?`||Query operation complete|

> `*IDN?` returns the current SCPI IDN configuration: `Manufacturer,Model,Serial Number,Firmware Version`

## System Commands

|Command|Parameter|Description|
|--|--|--|
|`SYSTem:ERRor[:NEXT]?`||Query next error message|
|`SYSTem:ERRor:COUNt?`||Query error count|
|`SYSTem:VERSion?`||Query firmware version and Git hash|
|`SYSTem:UPTime?`||Query system uptime in seconds|

### Example

```scpi
SYSTem:ERRor[:NEXT]?    # Query next error
SYSTem:ERRor:COUNt?     # Query error count
SYSTem:VERSion?         # Query firmware version
SYSTem:UPTime?          # Query uptime
```

## Device Identification Configuration Commands

|Command|Parameter|Description|
|--|--|--|
|`SYSTem:IDN1`|`<string>`|Configure manufacturer field, saved to Flash|
|`SYSTem:IDN1?`||Query manufacturer field|
|`SYSTem:IDN2`|`<string>`|Configure model field, saved to Flash|
|`SYSTem:IDN2?`||Query model field|
|`SYSTem:IDN3`|`<string>`|Configure serial number/date field, saved to Flash|
|`SYSTem:IDN3?`||Query serial number/date field|
|`SYSTem:IDN4`|`<string>`|Configure firmware version field, saved to Flash|
|`SYSTem:IDN4?`||Query firmware version field|

### Example

```scpi
SYSTem:IDN3?    # Query current serial number/date field
SYSTem:IDN4?    # Query current firmware version field
*IDN?           # Query all current IDN fields
```

## Communication Configuration Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:BAUDrate`|`115200`|Configure BSM baud rate. Only `115200` is supported|

### Example

```scpi
CONFigure:BAUDrate 115200
```

## Door Open/Close Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:CYLInder1`|`OPEN` / `CLOSE`|Open or close the door|
|`READ:CYLInder1:STATe?`|[Return Value](#actuator-state)|Query current door state|

## USB Plug/Unplug Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:CYLInder2`|`OPEN` / `CLOSE`|Unplug or plug the USB connector|
|`READ:CYLInder2:STATe?`|[Return Value](#actuator-state)|Query current USB state|
|`CONFigure:USB:AUTO`|`OFF` / `ON`|Configure factory USB automatic sequence setting; saved to Flash|
|`READ:USB:AUTO?`||Query the USB automatic sequence setting|
|`CONFigure:DUT:AUTO`|`OFF` / `ON`|Configure whether DUT presence detection participates in the USB automatic sequence; saved to Flash|
|`READ:DUT:AUTO?`||Query whether DUT presence detection participates in the USB automatic sequence|

> USB commands are named by connection state: `CYLInder2 CLOSE` plugs/connects USB, and `CYLInder2 OPEN` unplugs/retracts USB.

### Actuator State

|Return Value|Description|
|--|--|
|`CLOSE`|Retract / close command accepted|
|`OPEN`|Extend / open command accepted|
|`CLOSING`|Retracting / closing in progress|
|`OPENING`|Extending / opening in progress|
|`CLOSED`|Retracted / closed|
|`OPENED`|Extended / opened|
|`CYL ERR`|Actuator error|

For `CYLInder2`, `CLOSING/CLOSED` means USB plugging/plugged, and `OPENING/OPENED` means USB unplugging/unplugged.

### USB Automatic Sequence

`CONFigure:USB:AUTO ON` links USB insertion/retraction to the door sequence. When `CONFigure:DUT:AUTO ON`, DUT presence detection participates in the automatic sequence.

|Scenario|Automatic Action|
|--|--|
|`CONFigure:USB:AUTO OFF`|Bypass USB automatic insertion/retraction and DUT presence detection; use the normal door sequence|
|`CONFigure:USB:AUTO ON` and `CONFigure:DUT:AUTO OFF`|DUT presence detection does not participate; after button confirmation, run the USB automatic insertion sequence directly|
|`CONFigure:USB:AUTO ON` and `CONFigure:DUT:AUTO ON`|Check `dut_sensor` first; insert USB only after the DUT is in position, then close after USB insertion is confirmed|
|DUT not in position|Do not insert USB or close the door; keep waiting. `READ:DUT:STATe?` still reports the actual sensor state|
|USB insertion not reached or sensor fault|Do not close the door; log `USB_INSERT_FAIL`; retract USB if the door is still open; fast-flash red|
|`COMPLETE` and open-door button pressed|Open the door first; after the door reaches open position and returns to `IDLE`, retract USB if still inserted|
|USB retraction not reached or output fault|Log `USB_RETRACT_FAIL`; fast-flash red; return to `IDLE` if needed|

> `CONFigure:USB:AUTO` is factory configured by device hardware. When `USB:AUTO OFF`, DUT detection is bypassed in the automatic flow. `CONFigure:DUT:AUTO` is a field setting saved to Flash and is effective only when `USB:AUTO ON`. Query commands are not bypassed: `READ:DUT:STATe?` always reads the actual `dut_sensor`.

> In `LOCAL` mode, a legal SCPI parameter for an ordinary door or USB action is rejected with the standard SCPI error `-201,"Invalid while in local"`. A genuinely illegal parameter value continues to use `-224,"Illegal parameter value"` (or the applicable range error); the two cases must not be treated as the same fault.

### Example

```scpi
CONFigure:CYLInder1 OPEN     # Open door
CONFigure:CYLInder1 CLOSE    # Close door
CONFigure:CYLInder2 CLOSE    # Plug USB
CONFigure:CYLInder2 OPEN     # Unplug USB
READ:CYLInder1:STATe?        # Query door state
READ:CYLInder2:STATe?        # Query USB state
READ:USB:AUTO?               # Query USB automatic sequence setting
CONFigure:DUT:AUTO ON        # Enable DUT detection participation
READ:DUT:AUTO?               # Query DUT automatic participation
READ:DUT:STATe?              # Query actual DUT sensor state
```

## Lock Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:LOCK`|[Lock State](#lock-state)|Configure device lock state|
|`READ:LOCK:STATe?`|[Return Value](#lock-state)|Query lock state|

### Lock State

|Parameter / Return Value|Description|
|--|--|
|`UNLOCK`|Unlock|
|`LOCKED`|Lock|
|`LOCK ERR`|Lock error|

### Example

```scpi
CONFigure:LOCK LOCKED    # Lock the device
CONFigure:LOCK UNLOCK    # Unlock the device
READ:LOCK:STATe?         # Query lock state
```

## LED Indicator Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:LED`|[LED State](#led-state)|Configure LED indicator|
|`READ:LED:STATe?`|[Return Value](#led-state)|Query LED state|

### LED State

|Parameter / Return Value|Description|
|--|--|
|`OFF`|LED off|
|`GREEN`|Green LED|
|`RED`|Red LED|
|`YELLOW`|Yellow LED|
|`LED ERR`|LED error|

### Example

```scpi
CONFigure:LED GREEN    # Set LED to green
CONFigure:LED RED      # Set LED to red
CONFigure:LED OFF      # Turn off LED
READ:LED:STATe?        # Query LED state
```

## System State Query Command

|Command|Parameter|Description|
|--|--|--|
|`READ:SYSTem:STATe?`||Query system state|
|`READ:DUT:STATe?`|`INPOS` / `OUTPOS`|Query actual DUT presence sensor state|

### System State Return Values

|Return Value|Description|
|--|--|
|`LOCK`|System is locked|
|`IDLE`|System is idle|
|`READY`|System is ready|
|`RUNNING`|System is running|
|`EMERGENCY`|Emergency stop triggered|
|`COMPLETE`|System operation complete|
|`SYS ERR`|System error|

### Example

```scpi
READ:SYSTem:STATe?    # Query system state
READ:DUT:STATe?       # Query actual DUT sensor state
```

> `READ:DUT:STATe?` reads `dut_sensor`: `INPOS` means DUT in position, and `OUTPOS` means DUT not in position. This query always remains valid, regardless of `USB:AUTO` or `DUT:AUTO` bypass settings.

## Control Mode Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:MODE`|`LOCAL` / `REMOTE` / `MIXED`|Switch the current control mode; runtime only, not saved to Flash|
|`READ:MODE?`||Query the current control mode|
|`CONFigure:MODE:ENABle`|`ON` / `OFF`|Configure whether control-mode switching is enabled; saved to Flash|
|`READ:MODE:ENABle?`||Query whether control-mode switching is enabled|

### Control Mode Permissions

|Mode|Physical-button ordinary actions|SCPI ordinary actions|Queries and safety logic|
|--|--|--|--|
|`LOCAL`|Allowed|Rejected|Always available|
|`REMOTE`|Blocked|Allowed|Always available|
|`MIXED`|Allowed|Allowed|Always available|

`LOCAL` is the local manual-control mode. Ordinary door and USB actions are accepted from the physical panel, while equivalent SCPI actions are rejected. `REMOTE` is the remote software-control mode. Ordinary door and USB actions are accepted from SCPI, while ordinary physical-button actions are blocked. `MIXED` is the compatibility mode and preserves the current behavior by allowing both sources.

Status, alarm, error-queue, log, IO, sensor, version, and diagnostic queries remain available in all three modes. Emergency stop, laser anti-pinch, RS485 fault protection, power-loss retraction/locking, and other safety state-machine behavior are not disabled by the control mode. Lock and unlock are handled as an independent safety/maintenance permission.

### Mode-Switch Enable

|Value|Description|
|--|--|
|`ON`|Allow `CONFigure:MODE` to switch between `LOCAL`, `REMOTE`, and `MIXED`|
|`OFF`|Lock the current mode and reject ordinary mode changes; `CONFigure:MODE LOCAL` and `CONFigure:MODE:ENABle ON` remain available as recovery paths|

`CONFigure:MODE:ENABle` is the persistent configuration item and is written to Flash. `CONFigure:MODE` changes only the current runtime mode and is not restored from Flash. After power-up, the current mode is always `MIXED`; only `mode_enable` is restored from Flash. The firmware rejects the unsafe `REMOTE + OFF` combination so that remote control cannot be permanently locked out.

### Example

```scpi
CONFigure:MODE:ENABle ON
CONFigure:MODE REMOTE
READ:MODE?
READ:MODE:ENABle?
CONFigure:MODE MIXED
CONFigure:MODE LOCAL
```

## IO Status Query Command

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:IO:PROBe`|`RAW` / `FORWARD`|Select whether IO queries use physical or forwarded states (runtime)|
|`READ:IO:PROBe?`||Query the current IO probe mode|
|`READ:IO:ALL?`||Query all IO states using the selected probe mode|

### Return Format

```text
IN:0xHH,0xHH OUT:0xHH,0xHH
```

### Example

```scpi
READ:IO:ALL?
```

`RAW` reports the values sampled directly from the IO expansion board.
`FORWARD` reports input values after the input state layer (including active
configured pulse latches) and output values after the output state layer
(including logical state retained for pulse outputs). Risk Mode does not
enable or disable input latching; it only selects the pressure-sensor close
confirmation path. The default mode after boot is
`RAW`; the mode is runtime-only and does not alter the state machine.

The desktop debug tool presents these controls as two IO tabs: `Probe` for
display and source selection, and `Configure` for per-point trigger types and
output pulse width.

```scpi
CONFigure:IO:PROBe RAW
READ:IO:PROBe?
CONFigure:IO:PROBe FORWARD
READ:IO:ALL?
```

## Emergency Stop Input Type Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:ESTOP:TYPE`|`NC` / `NO`|Configure emergency stop input type, saved to Flash|
|`CONFigure:ESTOP:TYPE?`||Query emergency stop input type|

### Emergency Stop Type

|Parameter / Return Value|Description|
|--|--|
|`NC` / `NO`|`NC`: normally closed; `NO`: normally open|

### Example

```scpi
CONFigure:ESTOP:TYPE NC
CONFigure:ESTOP:TYPE?
```

## Risk Mode Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:RISK:MODE`|`OFF` / `ON`|Configure risk mode, saved to Flash|
|`CONFigure:RISK:MODE?`||Query risk mode|

### Risk Mode

|Parameter / Return Value|Description|
|--|--|
|`OFF` / `ON`|`OFF`: risk mode disabled; `ON`: risk mode enabled|

> Recommended input air pressure: about `0.33 MPa`; set the electronic pressure gauge threshold to about `0.3 MPa`.

### Example

```scpi
CONFigure:RISK:MODE ON
CONFigure:RISK:MODE?
```

## Input Trigger Type Commands

Each of the 16 input points can be configured independently. The configuration
is saved to Flash. All points default to `LEVEL`; applications can override the
compile-time default pulse mask.

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:INPut#:TYPE`|`LEVEL` / `PULSE`|Configure input point `#` (`1` to `16`)|
|`READ:INPut#:TYPE?`||Query input point `#` trigger type|

|Type|Behavior|
|--|--|
|`LEVEL`|Forward the current input level directly|
|`PULSE`|Latch a detected high pulse until the application releases that point|

Pulse latching is controlled by the configured input point type and is
independent of Risk Mode. The door and USB paired position sensors are
captured only while their corresponding actuator is moving toward that
position; the reverse actuator only enables the release path. Risk Mode remains
limited to the pressure-sensor close confirmation path. The reusable layer
accepts an application-defined capture and release policy for other input
points; output points remain direct actuator commands and are not processed by
the input state layer.

For the built-in door and USB position paths, the first debounced rising edge
on the allowed movement direction sets the latch. The reverse movement must
then produce a new low-to-high edge on the same sensor to clear it; changing
direction alone does not clear the latch, and a continuously high sensor does
not retrigger it.

```scpi
CONFigure:INPut1:TYPE PULSE
READ:INPut1:TYPE?
```

## Output Trigger Type Commands

Each of the 16 output points can be configured independently. The setting and
the common pulse width are saved to Flash. All output points default to
`LEVEL`; the current application therefore keeps its existing output behavior.

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:OUTPut#:TYPE`|`LEVEL` / `PULSE`|Configure output point `#` (`1` to `16`)|
|`READ:OUTPut#:TYPE?`||Query output point `#` trigger type|
|`CONFigure:OUTPut:PULSe:WIDTh`|`1` to `60000`|Set common pulse width in milliseconds|
|`READ:OUTPut:PULSe:WIDTh?`||Query common pulse width in milliseconds|

`LEVEL` forwards the requested output state continuously. `PULSE` emits one
high pulse on a rising logical request and automatically returns the physical
point low after the configured width. The logical output remains asserted until
the application requests it low, so repeated high requests do not retrigger it.

```scpi
CONFigure:OUTPut2:TYPE PULSE
CONFigure:OUTPut:PULSe:WIDTh 100
READ:OUTPut2:TYPE?
READ:OUTPut:PULSe:WIDTh?
```

## Revision History

|Date|Revision|
|--|--|
|`2026-09-24`|Consolidated the day's IO update: added switchable `RAW` / `FORWARD` probing (`CONFigure:IO:PROBe`, `READ:IO:PROBe?`, and `READ:IO:ALL?`), persistent per-input and per-output `LEVEL` / `PULSE` configuration, common output pulse width, direction-scoped input latch/release paths, and the Tools Probe/Configure UI. Input latching is independent of Risk Mode; Risk Mode remains limited to pressure-sensor close confirmation, and existing applications remain `LEVEL` by default.|
|`2026-09-12`|Added the implemented and real-device-verified `LOCAL` / `REMOTE` / `MIXED` control modes, mode-switch enable persistence, recovery paths, permission matrix, and the distinction between `-201,"Invalid while in local"` and `-224,"Illegal parameter value"`.|
|`2026-08-11`|Added DUT presence participation control for the USB automatic sequence and clarified that the DUT state query always reads the actual sensor.|
|`2026-07-28`|Revised USB manual-control and state-return semantics.|
