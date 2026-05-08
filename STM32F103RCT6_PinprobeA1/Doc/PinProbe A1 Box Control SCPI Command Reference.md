## PinProbe A1 Box Control SCPI Command Reference

---

## Device Information

|Item|Description|
|--|--|
|Manufacturer|GTS|
|Model|PINPROBEA1|
|Serial Number|20250626|
|Firmware Version|V0.0.1|

## Basic Commands (IEEE 488.2 Mandated Commands)

|Command|Parameter|Description|
|--|--|--|
|`*CLS`||Clear Status|
|`*IDN?`||Query Device Identification|
|`*RST`||Reset Device|
|`*STB?`||Query Status Byte|
|`*WAI`||Wait for Command Completion|
|`*OPC?`||Query Operation Complete|

> `*IDN?` returns: `GTS,PINPROBEA1,20250626,V0.0.1`

## System Commands

|Command|Parameter|Description|
|--|--|--|
|`SYSTem:ERRor[:NEXT]?`||Query Next Error Message|
|`SYSTem:ERRor:COUNt?`||Query Error Count|

## Door Open/Close Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:CYLInder1`|`OPEN` / `CLOSE` or [Actuator State](#actuator-state)|Open or close the door|
|`READ:CYLInder1:STATe?`|[Return Value](#actuator-state)|Query current door state|

## USB Plug/Unplug Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:CYLInder2`|`OPEN` / `CLOSE` or [Actuator State](#actuator-state)|Unplug or plug the USB connector|
|`READ:CYLInder2:STATe?`|[Return Value](#actuator-state)|Query current USB state|

### Actuator State

|Parameter|Description|
|--|--|
|`CLOSE`|Retract / Close (door closes / USB plugs)|
|`OPEN`|Extend / Open (door opens / USB unplugs)|
|`CLOSING`|Retracting / Closing in progress|
|`OPENING`|Extending / Opening in progress|
|`CLOSED`|Retracted / Closed|
|`OPENED`|Extended / Opened|
|`CYL ERR`|Actuator error|

### Example

```
CONFigure:CYLInder1 OPEN    # Open door
CONFigure:CYLInder1 CLOSE   # Close door
CONFigure:CYLInder2 OPEN    # Unplug USB
CONFigure:CYLInder2 CLOSE   # Plug USB
READ:CYLInder1:STATe?       # Query door state
READ:CYLInder2:STATe?       # Query USB state
```

## Lock Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:LOCK`|[Lock State](#lock-state)|Configure Device Lock State|
|`READ:LOCK:STATe?`|[Return Value](#lock-state)|Query Lock State|

### Lock State

|Parameter|Description|
|--|--|
|`UNLOCK`|Unlock|
|`LOCKED`|Lock|
|`LOCK ERR`|Lock error|

### Example

```
CONFigure:LOCK LOCKED    # Lock the device
CONFigure:LOCK UNLOCK    # Unlock the device
READ:LOCK:STATe?         # Query lock state
```

## LED Indicator Control Commands

|Command|Parameter|Description|
|--|--|--|
|`CONFigure:LED`|[LED State](#led-state)|Configure LED Indicator|
|`READ:LED:STATe?`|[Return Value](#led-state)|Query LED State|

### LED State

|Parameter|Description|
|--|--|
|`OFF`|LED off|
|`GREEN`|Green LED|
|`RED`|Red LED|
|`YELLOW`|Yellow LED|
|`LED ERR`|LED error|

### Example

```
CONFigure:LED GREEN    # Set LED to green
CONFigure:LED RED      # Set LED to red
CONFigure:LED OFF      # Turn off LED
READ:LED:STATe?        # Query LED state
```

## System State Query Command

|Command|Parameter|Description|
|--|--|--|
|`READ:SYSTem:STATe?`||Query System State|

### System State Return Values

|Return Value|Description|
|--|--|
|`LOCK`|System is locked|
|`IDLE`|System is idle|
|`READY`|System is ready|
|`RUNNING`|System is running|
|`EMERGENCY`|Emergency stop triggered (light curtain or emergency stop pressed)|
|`COMPLETE`|System operation complete|
|`SYS ERR`|System error|

### Example

```
READ:SYSTem:STATe?    # Query system state
```
