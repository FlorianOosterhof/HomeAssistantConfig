# Design of the "Total domestic hot water gas consumed" sensor:
# There are 3 sensors:
# - Binary sensor: DHW active
# - Binary sensor: Flame on
# - Sensor: total gas consumed
# Assumptions: Whenever the "total gas consumed" sensor is increased, it has because the "Flame on" sensor was on
# for some time. The domestic hot water gas consumed is calculated as:
# - Whenever the "gas consumed" sensor is updated by value G between timestamps T0 and T1, calculate the ratio:
#     R = (time between T0 and T1 that both "DHW active" and "Flame on" were on) /
#         (time between T0 and T1 that "Flame on" was on)
# - Increment the "Total domestic hot water gas consumed" sensor by G * R.
# Note: Since "total gas consumed" was increased, the denominator of the ratio R is non-zero, so the calculation
# is not undefined.

# Design of the template to calculate the "total domestic hot water gas consumed".
# Auxillary sensors:
# - Simplify the calculations by making a cumulative sensor of the binary sensors "DHW active" and "Flame on", with
#   states 00 (0, both off), 01 (1, flame on), 10 (2, dhw on), 11 (3, both on)
# - Trigger when the cumulative sensor has changed state for at least 1 second. This is done since the 2 binary
#   sensors may change state "at the same time" (they are the result of 1 OpenTherm Write/Read with multiple binary
#   sensors as output, so both of their states may change very shortly after another). Due to the async nature of
#   HA, the template may then calculate twice simultaneously. By requiring the cumulative state to have been reached
#   for at least 1 second, the trigger will fire only once.
# - Track the relevant binary sensor times in separate sensors that update when the relevant binary sensor changes
#   from "on" to another state.
# With these sensors, the ratio R can at any time be readily calculated as long as the current time is available:
#   The numerator and denominator are the value of the time-tracking sensor, plus if the binary sensor is on, the
#   time between its last_changed value and the current time.
# Therefore, the template triggers on:
# - Binary sensor changes from "on" to another state, which performs:
#   - Update of the relevant time-tracking sensor
# - Total gas consumed state changes (from TG to TG + G), which performs:
#   - Calculation of the numerator and denominator of the ratio R, which is an update of the auxillary
#     time-tracking sensors, and these are actually updated
#   - Calculation of the ratio R and storing it in another auxillary sensor.
#   - Increment the "DHW gas consumed" sensor by G * R
# - DHW gas consumed state changes:
#   - Reset the time-tracking sensors to 0, so that the ratio is calculated anew starting from the moment the last
#     total gas consumed update was done.
#
# An important implementation detail is: Updates of the time-tracking sensors based on the binary-sensor change
# from "on" to a different should not be done simply based on the time the binary sensor was in "on" state,
# because during that time there might have been updates to the total gas consumed sensor. Instead, it should be
# the time between the state change from "on" to a different state and the maximum of the following 2 timestamps:
# - The time at which the binary sensor changed to "on"
# - The last time the time-tracking sensor changed state.
# Since the time-tracking sensor only changes when the binary sensor updates or when it is reset to 0, this
# ensures the time-tracking sensor tracks only the binary sensor "on" time since the last "total gas consumed"
# state change.
