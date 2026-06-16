## PinProbe A1 Box Control SCPI Command Reference

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

### Example

```scpi
CONFigure:CYLInder1 OPEN     # Open door
CONFigure:CYLInder1 CLOSE    # Close door
CONFigure:CYLInder2 OPEN     # Unplug USB
CONFigure:CYLInder2 CLOSE    # Plug USB
READ:CYLInder1:STATe?        # Query door state
READ:CYLInder2:STATe?        # Query USB state
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
```

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

### Example

```scpi
CONFigure:RISK:MODE ON
CONFigure:RISK:MODE?
```
