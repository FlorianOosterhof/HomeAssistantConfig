Design of the "Total domestic hot water gas consumed" sensor
------------------------------------------------------------

The goal of this set of template sensors is to estimate how much of the consumed natural gas is used for central heating and how much for domestic hot water (DHW).

HA knows how much gas is consumed in total from a DSMR/ESMR reading every 5 minutes, and what the boiler is doing based on sniffing the OpenTherm protocol. The following 3 sensors will be used to estimate the DHW gas consumed:
- Binary sensor: DHW active
- Binary sensor: Flame on
- Sensor: total gas consumed

From here, the DHW gas consumed (sensor value) is calculated as:
- Whenever the "gas consumed" sensor is updated by value G between timestamps T0 and T1, calculate the ratio:
  * R = (time between T0 and T1 that both "DHW active" and "Flame on" were on) / (time between T0 and T1 that "Flame on" was on)
- Increment the "Total domestic hot water gas consumed" sensor by G * R.

This means that the amount of gas used for domestic hot water will typically be underestimated: If during a period of 5 minutes gas was used for both central heating and domestic hot water, the gas is split according to how much time the flame was on for each. However, for central heating the flame is typically modulated, meaning not the full power is used by opening the gas valve only partially, thus using less gas and heating the water more slowly. This in contrast with domestic hot water, which typically requires much power, i.e. the gas valve is opened (nearly) fully.

Therefore, during 5-minute windows where gas is used for only one of the 2, the estimate is correct, but if both uses were present in the time window, the gas usage for domestic hot water is typically underestimated. This could typically be the case at the start and end of taking a shower, or when the faucet in the bathroom is on, which is typically on for short periods of time.

Note on the calculation of R: When "total gas consumed" was increased, it could be assumed that the "Flame on" sensor has been on for at least some time. However, the OpenTherm protocol exchanges one message (with 1 "register") every 1 second. At the time of writing, the message containing both binary sensors is exchanged every 4 messages (i.e. every 4 seconds). It could be that the flame was on for less time, and thus the "Flame on" sensor is not actually on even though gas was consumed. However, since apparently the gas was on for a very short duration of time, the amount used should be negligible, and since the goal is an estimate, it is safe to set the ratio to 0 in case the denominator is 0, leading to the consumed gas being considered as used for central heating.

These 2 considerations result in underestimation of the gas used for domestic hot water. Nevertheless, it is a better estimate than 0% for domestic hot water and 100% for central heating.

Implementation details
----------------------

The following helper sensors are created:
- Simplify the calculations by making a cumulative sensor of the binary sensors "DHW active" and "Flame on", with states:
  - 0b00 = 0 = both off
  - 0b01 = 1 = flame on
  - 0b10 = 2 = dhw on
  - 0b11 = 3 = both on
- Trigger when the cumulative sensor has changed state for at least 0.5 second. This is done since the 2 binary sensors may change state "at the same time" (they are the result of 1 OpenTherm Write/Read with multiple binary sensors as output, so both of their states may change very shortly after another). Due to the async nature of HA, the template may then calculate twice simultaneously. By requiring the cumulative state to have been reached for at least 0.5 second, the trigger will fire only once.
- Track the relevant binary sensor times in separate sensors that update when the cumulative state sensor changes value.
  - Add an attribute "status" to one of the 2 time-tracking sensors, that is a fixed string when accumulating, and equal to the increase of gas when the gas consumed sensor changes.
- Track the ratio R in a separate sensor.
  - Add an attribute "status" that mimics the "status" attribute of the time-tracking sensor.

This way, the following sensors and triggers can be created:
- Cumulative sensor:
  - Triggers:
    - State changes of "DHW active" and "Flame on" binary sensors
    - Every X seconds
  - Value:
    - (2 if "DHW active" else 0) + (1 if "Flame on" else 0)
- Debounced Cumulative sensor:
  - Triggers:
    - State changes of Cumulative sensor for at least 0.5 seconds.
  - Value:
    - Equal to "to_state".
- 2 Time-tracking sensors (T1, T2):
  - Triggers:
    - State changes of:
      - Debounced cumulative sensor
      - Gas consumed sensor
    - Attribute "status" change of: Ratio sensor
  - Values:
    - On state change trigger: Increment by relevant amount of time
    - On Ratio sensor attribute change: 0
  - Attribute:
    - T1:
      - "status":
        - On Gas consumed sensor change: "(new state) - (old state)"
        - Else: string value "accumulating"
- Ratio sensor:
  - Triggers:
    - Attribute "status" change of: Time-tracking sensor
    - Every X seconds
  - Value:
    - (T1 sensor value) / (T1 sensor value + T2 sensor value), or 0 when both are 0.
  - Attribute:
    - "status" equal to the attribute "status" of T1.
- DHW/CH gas consumed:
  - Triggers:
    - Attribute "status" change of: Ratio sensor
  - Values:
    - DHW gas consumed: Increment by (R sensor value) * (attribute "status" or R sensor)
    - CH gas consumed: (gas consumed sensor value) - (DHW gas consumed sensor value)

In total the following triggers are applicable:
- State change of "DHW active" or "Flame on" binary sensors:
  - Update of Cumulative sensor:
    - Update of Debounced Cumulative sensor:
      - Update of Time-tracking sensors: increments them, and always sets the "status" attribute to "accumulating"
- State change of "gas consumed" sensor:
  - Update of Time-tracking sensors: Increment the time-tracking sensors and set "status" attribute to the gas consumed sensor increment.
    - Update of Ratio sensor: Update the ratio and sets "status" attribute to the gas consumed sensor increment.
      - Update of Time-tracking sensors: Reset to 0 and set "status" attribute to "accumulating"
      - Update of DHW/CH gas consumed sensors
- Trigger every X seconds:
  - Update of Cumulative sensor:
    - Update of Debounced Cumulative sensor:
      - Update of Time-tracking sensors: increments them, and always set the "status" attribute to "accumulating"
  - Update of Ratio sensor: updates the ratio and always sets the "status" attribute to "accumulating"

The "Trigger every X seconds" triggers are added for easier debugging of the sensors by updating the time-tracking sensors and ratio at a consistent interval so that their values represent the "current state" accurately, even if no changes of the binary sensors or gas consumed sensor are taking place, e.g. when during the entire 5-minute time window between gas consumed sensor updates the shower is on.