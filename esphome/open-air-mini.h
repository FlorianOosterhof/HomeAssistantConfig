#include <cmath>

inline float convert_rpm_to_delta_temp(const float rpm)
{
    return 2.8867094017094136e-001 + 1.5779059829059802e-003 * rpm - 3.3418803418803323e-007 * rpm * rpm;
}

inline float calculate_saturation_water_vapor_pressure(const float temp)
{
    return 0.61078 * std::exp(17.27 * temp/(temp + 237.3));
}

inline float correct_temperature(const float old_temp, const float rpm)
{
    const float new_temp = old_temp + convert_rpm_to_delta_temp(rpm);
    return new_temp;
}

inline float correct_humidity(const float old_hum, const float old_temp, const float rpm)
{
    const float new_temp = correct_temperature(old_temp, rpm);
    const float old_press = calculate_saturation_water_vapor_pressure(old_temp);
    const float new_press = calculate_saturation_water_vapor_pressure(new_temp);
    const float new_hum_raw = old_hum * old_press / new_press;
    const float new_hum = std::fmin(100.0, std::fmax(0.0, new_hum_raw));
    return new_hum;
}