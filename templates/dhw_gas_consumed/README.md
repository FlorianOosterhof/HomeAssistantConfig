Design of the "Total domestic hot water gas consumed" sensor
------------------------------------------------------------

The goal of this set of template sensors is to estimate how much of the consumed natural gas is used for central heating and how much for domestic hot water (DHW).

HA knows how much gas is consumed in total from a DSMR/ESMR reading every 5 minutes, and what the boiler is doing based on sniffing the OpenTherm protocol. The following 3 sensors will be used to estimate the DHW gas consumed:
- Binary sensor: DHW active
- Binary sensor: Flame on
- Sensor: total gas consumed

From here, the DHW gas consumed (sensor value) is calculated as:
- Whenever the "gas consumed" sensor is updated by value G between timestamps T0 and T1, calculate the ratio:
    R = (time between T0 and T1 that both "DHW active" and "Flame on" were on) / (time between T0 and T1 that "Flame on" was on)
- Increment the "Total domestic hot water gas consumed" sensor by G * R.

Note: Since "total gas consumed" was increased, it could be assumed that the "Flame on" sensor has been on for at least some time. However, the OpenTherm protocol exchanges one message (with 1 "register") every 1 second. At the time of writing, the message containing both binary sensors is exchanged every 4 messages. It could be that the flame was on for less time, and thus the "Flame on" sensor is not actually on even though gas was consumed. However, since apparently the gas was on for a very short duration of time, the amount used should be negligible, and since the goal is an estimate, it is safe to set the ratio to 0 in case the denominator is 0, leading to the consumed gas being considered as used for central heating.

The following helper sensors are created:
- Simplify the calculations by making a cumulative sensor of the binary sensors "DHW active" and "Flame on", with
  states 00 (0, both off), 01 (1, flame on), 10 (2, dhw on), 11 (3, both on)
- Trigger when the cumulative sensor has changed state for at least 0.5 second. This is done since the 2 binary sensors may change state "at the same time" (they are the result of 1 OpenTherm Write/Read with multiple binary sensors as output, so both of their states may change very shortly after another). Due to the async nature of HA, the template may then calculate twice simultaneously. By requiring the cumulative state to have been reached for at least 0.5 second, the trigger will fire only once.
- Track the relevant binary sensor times in separate sensors that update when the cumulative state sensor changes value.
- Track the ratio R in a separate sensor.

With these sensors, the ratio R can at any time be readily calculated as long as the current time is available:
  The numerator and denominator are the value of the time-tracking sensor plus, if the cumulative state has the correct value, the time between now and the last change of the time tracking sensor.
Therefore, the template triggers on:
- Cumulative state sensor changes
- Time pattern
- Total gas consumed state changes
- Attribute "status" of the ratio sensor.
The first three triggers update the time-tracking sensors and the sensor that is the ratio R. The third trigger sets the "status" attribute of the ratio sensor to "final", indicating everything needs to be reset. The fourth trigger fires on that event, and can then reset the time-tracking sensors and the ratio.
