// Blacklight model animation. Simulation direction and camera state stay separate.
// Licensed under the project's GNU GPL version 2 or later.
#ifndef ARMAGETRON_LIGHTCYCLE_MOTION_H
#define ARMAGETRON_LIGHTCYCLE_MOTION_H

#include <algorithm>
#include <cmath>

namespace gLightcycleMotion
{
constexpr double Pi = 3.14159265358979323846;
// A readable carved pose: roughly 200 ms to 95% of a quarter turn. The body
// carries angular velocity through rapid input; banking follows instead of
// kicking instantly. These are presentation time constants, not steering rules.
constexpr double YawResponse = 24.0;
constexpr double BankResponse = 16.0;
constexpr double BankFromYawSpeed = .030;
constexpr double MaximumBank = .42;
constexpr double PositionResponse = 28.0;
constexpr double MaximumVisualOffset = .35;

// Start construction after the first resource-loading frame. A cold shader or
// mesh upload must not consume the whole animation between two presentations.
// Already-established cycles remain assembled when a spectator joins late.
class AssemblyClock
{
public:
    void Reset()
    {
        seen_ = started_ = skip_ = false;
        last_ = 0;
    }
    double Sample(double now, double actualAge)
    {
        if (!std::isfinite(now) || !std::isfinite(actualAge))
            return 1.25;
        if (seen_ && now < last_)
            Reset();
        if (actualAge < 0)
        {
            Reset();
            return actualAge;
        }
        last_ = now;
        if (!seen_)
        {
            seen_ = true;
            first_ = now;
            skip_ = actualAge >= 1.25;
            return skip_ ? actualAge : 0;
        }
        if (skip_)
            return actualAge;
        if (!started_)
        {
            if (now <= first_)
                return 0;
            started_ = true;
            epoch_ = now;
        }
        return std::max(0., now - epoch_);
    }

private:
    bool seen_ = false, started_ = false, skip_ = false;
    double first_ = 0, epoch_ = 0, last_ = 0;
};

inline double Wrap(double angle)
{
    return std::remainder(angle, 2 * Pi);
}

// The rendered model is half its authored size. Distance-based phase avoids the
// old atan(step) approximation and keeps the equal-size wheels synchronized.
inline double WheelAngle(double distance, double modelRadius, double modelScale = 1)
{
    return std::isfinite(distance) && std::isfinite(modelRadius) && std::isfinite(modelScale) && modelRadius > 0 && modelScale > 0
               ? Wrap(2 * distance / (modelRadius * modelScale))
               : 0;
}

class State
{
public:
    void Reset() { initialized_ = false; }

    void Update(double now, double target, double turnTime, double speed = 0)
    {
        if (!std::isfinite(now) || !std::isfinite(target))
            return;
        target = Wrap(target);
        if (!initialized_ || now < sampleTime_ || now - sampleTime_ > .5)
        {
            initialized_ = true;
            sampleTime_ = startTime_ = now;
            target_ = startYaw_ = yaw_ = target;
            startVelocity_ = bank_ = startBank_ = 0;
            offsetTime_ = now;
            offsetStartX_ = offsetStartY_ = offsetVelocityX_ = offsetVelocityY_ = offsetX_ = offsetY_ = 0;
            return;
        }
        if (std::abs(Wrap(target - target_)) > 1e-6)
        {
            // Start at the accepted turn's time, not the first frame that sees it.
            double start = std::isfinite(turnTime)
                               ? std::max(sampleTime_, std::min(now, turnTime))
                               : now;
            Pose from = PoseAt(start);
            Offset offset = OffsetAt(start);
            // Feed forward the velocity before the accepted corner, then let
            // the drawn body catch the authoritative path. This creates a short
            // curved sweep without changing physics, timing or travelled distance.
            double travel = std::isfinite(speed) ? std::max(0., std::min(100., speed)) : 0;
            offsetStartX_ = offset.x;
            offsetStartY_ = offset.y;
            offsetVelocityX_ = offset.vx + (std::cos(target_) - std::cos(target)) * travel;
            offsetVelocityY_ = offset.vy + (std::sin(target_) - std::sin(target)) * travel;
            offsetTime_ = start;
            // Keep the accepted turn direction when another input arrives
            // before the bike finishes its previous pose. Rebase the angle at
            // wrap without discarding angular velocity or the existing lean.
            double nextTarget = target_ + Wrap(target - target_);
            startTime_ = start;
            startYaw_ = target + (from.yaw - nextTarget);
            startVelocity_ = from.velocity;
            startBank_ = from.bank;
            target_ = target;
        }
        Pose pose = PoseAt(now);
        yaw_ = Wrap(pose.yaw);
        // A soft bound avoids the visible flat top of an additive bank clamp.
        bank_ = MaximumBank * std::tanh(pose.bank / MaximumBank);
        Offset offset = OffsetAt(now);
        double length = std::hypot(offset.x, offset.y);
        double scale = length > 1e-12 ? MaximumVisualOffset * std::tanh(length / MaximumVisualOffset) / length : 1;
        offsetX_ = offset.x * scale;
        offsetY_ = offset.y * scale;
        sampleTime_ = now;
    }

    double Yaw() const { return yaw_; }
    double Bank() const { return bank_; }
    double VisualOffsetX() const { return offsetX_; }
    double VisualOffsetY() const { return offsetY_; }

private:
    struct Pose
    {
        double yaw, velocity, bank;
    };
    struct Offset
    {
        double x, y, vx, vy;
    };

    Offset OffsetAt(double now) const
    {
        double t = std::max(0., now - offsetTime_), decay = std::exp(-PositionResponse * t);
        double xSlope = offsetVelocityX_ + PositionResponse * offsetStartX_;
        double ySlope = offsetVelocityY_ + PositionResponse * offsetStartY_;
        return {(offsetStartX_ + xSlope * t) * decay, (offsetStartY_ + ySlope * t) * decay,
                (offsetVelocityX_ - PositionResponse * xSlope * t) * decay,
                (offsetVelocityY_ - PositionResponse * ySlope * t) * decay};
    }

    Pose PoseAt(double now) const
    {
        double age = std::max(0., now - startTime_);
        double offset = startYaw_ - target_;
        double decay = std::exp(-YawResponse * age);
        double velocitySlope = -YawResponse * (startVelocity_ + YawResponse * offset);
        Pose result;
        result.yaw = target_ + (offset + (startVelocity_ + YawResponse * offset) * age) * decay;
        result.velocity = (startVelocity_ + velocitySlope * age) * decay;

        // Bank follows the actual animated angular velocity through a slower
        // roll response. This is the exact exponential integral, so it remains
        // identical across frame rates and repeated mirror/main render passes.
        // Retargeting preserves both the angle and its velocity, and therefore
        // both the bank and its slope: quick reversals cannot kick the rider.
        double bankDecay = std::exp(-BankResponse * age);
        double rateDifference = BankResponse - YawResponse;
        double velocityIntegral = startVelocity_ * (decay - bankDecay) / rateDifference;
        velocityIntegral += velocitySlope * ((rateDifference * age - 1) * decay + bankDecay) / (rateDifference * rateDifference);
        result.bank = startBank_ * bankDecay - BankFromYawSpeed * BankResponse * velocityIntegral;
        return result;
    }
    bool initialized_ = false;
    double sampleTime_ = 0, startTime_ = 0;
    double target_ = 0, startYaw_ = 0, startVelocity_ = 0, yaw_ = 0;
    double startBank_ = 0, bank_ = 0;
    double offsetTime_ = 0, offsetStartX_ = 0, offsetStartY_ = 0, offsetVelocityX_ = 0, offsetVelocityY_ = 0;
    double offsetX_ = 0, offsetY_ = 0;
};
} // namespace gLightcycleMotion
#endif
