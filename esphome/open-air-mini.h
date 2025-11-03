#include <cmath>

inline double convert_rpm_to_delta_temp(const double rpm)
{
    return 2.8867094017094136e-001 + 1.5779059829059802e-003 * rpm - 3.3418803418803323e-007 * rpm * rpm;
}

inline double calculate_saturation_water_vapor_pressure(const double temp)
{
    return 0.61078 * std::exp(17.27 * temp/(temp + 237.3));
}

inline double correct_temperature(const double old_temp, const double rpm)
{
    const double new_temp = old_temp + convert_rpm_to_delta_temp(rpm);
    return new_temp;
}

inline double correct_humidity(const double old_hum, const double old_temp, const double new_temp, const double rpm)
{
    const double old_press = calculate_saturation_water_vapor_pressure(old_temp);
    const double new_press = calculate_saturation_water_vapor_pressure(new_temp);
    const double new_hum_raw = old_hum * old_press / new_press;
    const double new_hum = std::fmin(100.0, std::fmax(0.0, new_hum_raw));
    return new_hum;
}