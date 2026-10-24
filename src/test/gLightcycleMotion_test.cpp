#include "gLightcycleMotion.h"
#include "doctest.h"
#include <cmath>
#include <limits>

namespace
{

void near(double a, double b) { CHECK((std::abs(a - b) < 1e-8)); }

TEST_CASE("gLightcycleMotion: continuous presentation and bounded geometry")
{
    using namespace gLightcycleMotion;
    AssemblyClock birth;
    near(birth.Sample(-2, -.75), -.75);
    near(birth.Sample(-1, .25), 0);
    near(birth.Sample(-1, .25), 0);  // mirror/main share the first presentation
    near(birth.Sample(.2, 1.45), 0); // a cold first upload cannot skip construction
    near(birth.Sample(.7, 1.95), .5);
    near(birth.Sample(.7, 1.95), .5);
    near(birth.Sample(1.7, 2.95), 1.5);
    birth.Reset();
    near(birth.Sample(12, 12), 12); // joining an established match never rebuilds it
    near(birth.Sample(12.1, 12.1), 12.1);
    near(birth.Sample(-.5, .75), 0); // new round resets presentation time
    State motion;
    motion.Update(.99, 0, 0);
    motion.Update(1.005, Pi / 2, 1);
    CHECK((motion.Yaw() > 0 && motion.Yaw() < Pi / 18)); // no 30-degree first-frame snap
    CHECK((motion.Bank() < 0));
    double firstYaw = motion.Yaw(), firstBank = motion.Bank();
    motion.Update(1.005, Pi / 2, 1); // reflection and main pass must agree
    near(motion.Yaw(), firstYaw);
    near(motion.Bank(), firstBank);
    motion.Update(1.205, Pi / 2, 1);
    CHECK((motion.Yaw() > Pi / 2 * .95 && motion.Yaw() < Pi / 2));
    motion.Update(1.290, Pi / 2, 1);
    CHECK((motion.Yaw() > Pi / 2 * .99 && motion.Yaw() < Pi / 2));
    motion.Update(1.40, Pi / 2, 1);
    motion.Update(1.98, Pi / 2, 1);
    near(motion.Yaw(), Pi / 2);
    CHECK((std::abs(motion.Bank()) < 1e-6));

    // Sampling at different frame rates follows the same accepted-turn timeline,
    // including irregular frames, reflection passes, and a quick second turn.
    State reference;
    reference.Update(.99, 0, 0);
    reference.Update(1.05, Pi / 2, 1);
    double referenceYaw = reference.Yaw(), referenceBank = reference.Bank();
    reference.Update(1.08, Pi, 1.075);
    double secondYaw = reference.Yaw(), secondBank = reference.Bank();
    for (int fps : {30, 60, 120, 240})
    {
        State sampled;
        sampled.Update(.99, 0, 0);
        for (int i = 1; double(i) / fps < .05; ++i)
        {
            sampled.Update(1 + double(i) / fps, Pi / 2, 1);
            sampled.Update(1 + double(i) / fps, Pi / 2, 1);
        }
        sampled.Update(1.05, Pi / 2, 1);
        near(sampled.Yaw(), referenceYaw);
        near(sampled.Bank(), referenceBank);
        sampled.Update(1.063, Pi / 2, 1);
        sampled.Update(1.08, Pi, 1.075);
        near(sampled.Yaw(), secondYaw);
        near(sampled.Bank(), secondBank);
    }

    // Retargeting used to restart the cubic yaw from zero velocity and add a
    // second bank kick. Check angle AND velocity continuity in both directions.
    for (double nextTarget : {0., Pi})
    {
        State interrupted;
        interrupted.Update(.99, 0, 0);
        interrupted.Update(1.035 - 1e-6, Pi / 2, 1);
        double beforeYaw = interrupted.Yaw(), beforeBank = interrupted.Bank();
        interrupted.Update(1.035, Pi / 2, 1);
        double atYaw = interrupted.Yaw(), atBank = interrupted.Bank();
        double velocityBefore = Wrap(atYaw - beforeYaw) / 1e-6;
        double rollBefore = (atBank - beforeBank) / 1e-6;
        interrupted.Update(1.035, nextTarget, 1.035);
        near(interrupted.Yaw(), atYaw);
        near(interrupted.Bank(), atBank);
        interrupted.Update(1.035 + 1e-6, nextTarget, 1.035);
        double velocityAfter = Wrap(interrupted.Yaw() - atYaw) / 1e-6;
        double rollAfter = (interrupted.Bank() - atBank) / 1e-6;
        CHECK((std::abs(velocityAfter - velocityBefore) < .02));
        CHECK((std::abs(rollAfter - rollBefore) < .002));
        interrupted.Update(1.435, nextTarget, 1.035);
        interrupted.Update(2.035, nextTarget, 1.035);
        interrupted.Update(2.135, nextTarget, 1.035);
        near(Wrap(interrupted.Yaw() - nextTarget), 0);
        CHECK((std::abs(interrupted.Bank()) < 1e-6));
    }

    // An isolated corner has no overshoot or excessive angular spike. Lean
    // develops gradually and returns with the turn, rather than clipping.
    State sweep;
    sweep.Update(.99, 0, 0);
    double previousYaw = 0, peakBank = 0;
    for (int i = 1; i <= 300; ++i)
    {
        sweep.Update(1 + i * .001, Pi / 2, 1);
        CHECK((sweep.Yaw() >= previousYaw && sweep.Yaw() <= Pi / 2));
        CHECK(((sweep.Yaw() - previousYaw) / .001 < 17));
        peakBank = std::max(peakBank, std::abs(sweep.Bank()));
        previousYaw = sweep.Yaw();
    }
    CHECK((peakBank > .20 && peakBank < .36));

    // Same-direction turns carry through the angle wrap; reversing shortly
    // afterwards stays finite and never exceeds the visual lean limit.
    State reversal;
    reversal.Update(.99, Pi - .1, 0);
    reversal.Update(1.01, -Pi + .1, 1);
    CHECK((std::abs(Wrap(reversal.Yaw() - (Pi - .1))) < .1));
    reversal.Update(1.04, Pi - .1, 1.04);
    CHECK((std::isfinite(reversal.Yaw()) && std::abs(reversal.Bank()) < MaximumBank));
    reversal.Update(1.44, Pi - .1, 1.04);
    reversal.Update(2.04, Pi - .1, 1.04);
    near(Wrap(reversal.Yaw() - (Pi - .1)), 0);
    State wrapped;
    wrapped.Update(.99, Pi - .1, 0);
    wrapped.Update(1.02, Wrap(Pi - .1 + Pi / 2), 1);
    double wrappedYaw = wrapped.Yaw();
    wrapped.Update(1.02, Wrap(Pi - .1 + Pi), 1.02);
    near(wrapped.Yaw(), wrappedYaw);
    wrapped.Update(1.021, Wrap(Pi - .1 + Pi), 1.02);
    CHECK((Wrap(wrapped.Yaw() - wrappedYaw) > 0));
    wrapped.Update(1.42, -.1, 1.02);
    wrapped.Update(2.02, -.1, 1.02);
    near(Wrap(wrapped.Yaw() + .1), 0);

    State repeated;
    repeated.Update(.99, 0, 0);
    for (int turn = 0; turn < 20; ++turn)
    {
        double turnAt = 1 + turn * .075;
        double target = Wrap((turn + 1) * Pi / 2);
        for (int frame = 0; frame < 15; ++frame)
        {
            repeated.Update(turnAt + frame * .005, target, turnAt);
            CHECK((std::isfinite(repeated.Yaw())));
            CHECK((std::abs(repeated.Bank()) < MaximumBank));
        }
    }
    reversal.Reset();
    reversal.Update(-3, .75, -4);
    near(reversal.Yaw(), .75);
    near(reversal.Bank(), 0);
    reversal.Update(-4, -.75, -4); // a new round never animates through the old pose
    near(reversal.Yaw(), -.75);
    near(reversal.Bank(), 0);
    reversal.Update(std::numeric_limits<double>::quiet_NaN(), 0, 0);
    near(reversal.Yaw(), -.75);

    near(WheelAngle(Pi * .72, .72), 0); // one world-space circumference
    near(WheelAngle(Pi * .72 / 4, .72), Pi / 2);
    near(WheelAngle(12.5, .72), WheelAngle(12.5 + 20 * Pi * .72, .72));
    near(WheelAngle(Pi * .72 * 1.4, .72, 1.4), 0);
    near(WheelAngle(Pi * .72 * 1.4 / 4, .72, 1.4), Pi / 2);
    near(WheelAngle(1, .72, 0), 0);
    near(WheelAngle(1, .72, std::numeric_limits<double>::quiet_NaN()), 0);

    // The visual corner starts in the incoming direction and returns to the
    // physical path. It is a bounded presentation offset, never extra distance.
    State path;
    path.Update(.99, 0, 0, 20);
    path.Update(1, Pi / 2, 1, 20);
    near(path.VisualOffsetX(), 0);
    near(path.VisualOffsetY(), 0);
    path.Update(1.005, Pi / 2, 1, 20);
    CHECK((path.VisualOffsetX() > 0 && path.VisualOffsetY() < 0));
    CHECK((.1 + path.VisualOffsetY() > 0));                    // already moving into the outgoing leg
    CHECK((path.VisualOffsetX() > .1 + path.VisualOffsetY())); // incoming tangent first
    double x = path.VisualOffsetX(), y = path.VisualOffsetY();
    path.Update(1.005, Pi / 2, 1, 20); // reflection/main cannot advance the curve
    near(path.VisualOffsetX(), x);
    near(path.VisualOffsetY(), y);
    path.Update(1.075, Pi / 2, 1, 20);
    x = path.VisualOffsetX();
    y = path.VisualOffsetY();
    path.Update(1.075, Pi, 1.075, 20);
    near(path.VisualOffsetX(), x);
    near(path.VisualOffsetY(), y);
    path.Update(1.45, Pi, 1.075, 20);
    CHECK((std::hypot(path.VisualOffsetX(), path.VisualOffsetY()) < .001));
    for (int fps : {30, 60, 120, 240})
    {
        State sampled;
        sampled.Update(.99, 0, 0, 20);
        for (int i = 1; double(i) / fps < .05; ++i)
            sampled.Update(1 + double(i) / fps, Pi / 2, 1, 20);
        sampled.Update(1.05, Pi / 2, 1, 20);
        State exact;
        exact.Update(.99, 0, 0, 20);
        exact.Update(1.05, Pi / 2, 1, 20);
        near(sampled.VisualOffsetX(), exact.VisualOffsetX());
        near(sampled.VisualOffsetY(), exact.VisualOffsetY());
    }
    for (int turn = 0; turn < 20; ++turn)
    {
        double at = 2 + turn * .075;
        path.Update(at, Wrap((turn + 1) * Pi / 2), at, 100);
        for (int frame = 0; frame < 15; ++frame)
        {
            path.Update(at + frame * .005, Wrap((turn + 1) * Pi / 2), at, 100);
            CHECK((std::hypot(path.VisualOffsetX(), path.VisualOffsetY()) <= MaximumVisualOffset));
        }
    }
}

} // namespace
