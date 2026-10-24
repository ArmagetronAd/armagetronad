// Frame-rate independent presentation for the local Blacklight chase camera.
// Licensed under the project's GNU GPL version 2 or later.
#ifndef ARMAGETRON_CAMERA_MOTION_H
#define ARMAGETRON_CAMERA_MOTION_H

#include <algorithm>
#include <cmath>

namespace eCameraMotion
{
constexpr double Pi = 3.14159265358979323846;

inline double Wrap(double angle)
{
    return std::remainder(angle, 2 * Pi);
}

// The continuous form of normalize(direction + target * rate * dt) obeys
// error' = -rate * sin(error). Integrating that response exactly preserves the
// existing turn-speed setting without making low-frame-rate turns slower.
inline double Follow(double current, double target, double lastDirection,
                     double seconds, double rate, double reversalBoost)
{
    if (!std::isfinite(current))
        return std::isfinite(target) ? Wrap(target) : 0;
    current = Wrap(current);
    if (!std::isfinite(target) || !std::isfinite(seconds) || !std::isfinite(rate) || seconds <= 0 || rate <= 0)
        return current;

    double error = Wrap(target - current);
    double boost = std::isfinite(reversalBoost) ? std::max(0., reversalBoost) : 0;
    if (std::cos(error) < 0 && boost > 1e-8)
    {
        // At a reversal, follow the previous driving direction around the
        // corner. It resolves the ambiguous 180-degree case and keeps a quick
        // second turn from choosing the opposite side midway through a swing.
        if (std::isfinite(lastDirection))
        {
            double side = std::sin(lastDirection - current);
            if (side > 1e-10 && error < 0)
                error += 2 * Pi;
            if (side < -1e-10 && error > 0)
                error -= 2 * Pi;
        }
        double sign = error < 0 ? -1. : 1.;
        double remaining = std::abs(error);
        double excess = std::max(0., remaining - Pi / 2);
        double root = std::sqrt(boost);

        // Behind the target use angular speed rate*(sin(error)^2 +
        // boost*cos(error)^2). It reaches the configured reversal boost at
        // 180 degrees and joins the ordinary response with matching speed and
        // slope at 90 degrees. The atan2 form integrates it without substeps.
        double phase = std::atan2(root * std::sin(excess), std::cos(excess));
        double behindTime = phase / (root * rate);
        if (seconds < behindTime)
        {
            phase -= seconds * root * rate;
            double nextExcess = std::atan2(std::sin(phase) / root, std::cos(phase));
            double nextError = Pi / 2 + nextExcess;
            return Wrap(current + sign * (remaining - nextError));
        }
        current = Wrap(current + sign * excess);
        seconds -= behindTime;
        error = sign * Pi / 2;
    }
    else if (std::abs(error) >= Pi - 1e-12)
    {
        // With reversal assistance disabled an exactly opposing target has no
        // tangential force, matching the ordinary normalized-vector response.
        return current;
    }

    double nextError = 2 * std::atan(std::tan(error / 2) * std::exp(-rate * seconds));
    return Wrap(current + error - nextError);
}
} // namespace eCameraMotion
#endif
