## PinProbe A1 Box Control SCPI Command Reference

> Document updated: `2026-08-11`. This document is intended for customer-side host integration, device control, and maintenance reference.

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

## IO Status Query Command

|Command|Parameter|Description|
|--|--|--|
|`READ:IO:ALL?`||Query all raw input and output states|

### Return Format

```text
IN:0xHH,0xHH OUT:0xHH,0xHH
```

### Example

```scpi
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
|`NC`|Normally closed|
|`NO`|Normally open|

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
|`OFF`|Risk mode disabled|
|`ON`|Risk mode enabled|

> Recommended input air pressure: about `0.33 MPa`; set the electronic pressure gauge threshold to about `0.3 MPa`.

### Example

```scpi
CONFigure:RISK:MODE ON
CONFigure:RISK:MODE?
```
