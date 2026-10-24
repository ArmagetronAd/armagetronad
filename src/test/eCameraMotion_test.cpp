#include "eCameraMotion.h"
#include "doctest.h"
#include <cmath>
#include <limits>

namespace
{

void near(double a, double b, double tolerance = 1e-10)
{
    CHECK((std::abs(eCameraMotion::Wrap(a - b)) < tolerance));
}

double sample(double current, double target, double previous, double duration,
              double rate, double boost, int fps)
{
    for (double elapsed = 0; elapsed < duration;)
    {
        double step = std::min(1. / fps, duration - elapsed);
        current = eCameraMotion::Follow(current, target, previous, step, rate, boost);
        elapsed += step;
    }
    return current;
}

TEST_CASE("eCameraMotion: continuous presentation and bounded geometry")
{
    using namespace eCameraMotion;
    double quarter = Follow(0, Pi / 2, 0, .1, 10, 4);
    CHECK((quarter > .8 && quarter < .95));
    // Preserve rate semantics: doubling rate halves the time for the same pose.
    near(quarter, Follow(0, Pi / 2, 0, .05, 20, 4));
    CHECK((Follow(0, Pi / 2, 0, .1, 4, 4) < quarter));

    for (int fps : {30, 60, 120, 240})
    {
        for (double duration : {.025, .075, .1, .25, 1.})
        {
            near(sample(0, Pi / 2, 0, duration, 10, 4, fps),
                 Follow(0, Pi / 2, 0, duration, 10, 4));
            // This covers the analytic reversal segment, crossing its 90-degree
            // join, and the ordinary response after that join in either direction.
            for (double previous : {Pi / 2, -Pi / 2})
                near(sample(0, Pi, previous, duration, 10, 4, fps),
                     Follow(0, Pi, previous, duration, 10, 4));
        }

        // A second turn arrives before the first settles, then the input reverses.
        double sampled = 0, reference = 0;
        double target[] = {Pi / 2, Pi, Pi / 2, 0, -Pi / 2, Pi};
        double previous[] = {0, Pi / 2, Pi, Pi / 2, 0, -Pi / 2};
        double duration[] = {.05, .07, .033, .025, .075, .2};
        for (int i = 0; i < 6; ++i)
        {
            sampled = sample(sampled, target[i], previous[i], duration[i], 10, 4, fps);
            reference = Follow(reference, target[i], previous[i], duration[i], 10, 4);
            near(sampled, reference);
        }
    }

    // LastDirection determines which side an exact half-turn takes. It also
    // remains stable if the camera was already slightly across the opposite side.
    CHECK((Follow(0, Pi, Pi / 2, .01, 10, 4) > 0));
    CHECK((Follow(0, Pi, -Pi / 2, .01, 10, 4) < 0));
    CHECK((Wrap(Follow(-.1, Pi, Pi / 2, .01, 10, 4) + .1) > 0));
    CHECK((Wrap(Follow(.1, Pi, -Pi / 2, .01, 10, 4) - .1) < 0));
    CHECK((std::abs(Wrap(Follow(0, Pi, Pi / 2, 1, 10, 4) - Pi)) < .001));
    CHECK((Follow(0, Pi, Pi / 2, .02, 10, 4) > Follow(0, Pi, Pi / 2, .02, 10, 1)));
    near(Follow(0, Pi, Pi / 2, .02, 10, 0), 0); // reversal assistance can be disabled

    // No velocity jump when the boosted half-turn enters the ordinary response.
    double join = Pi / (2 * std::sqrt(4.) * 10), epsilon = 1e-7;
    double before = Follow(0, Pi, Pi / 2, join - epsilon, 10, 4);
    double at = Follow(0, Pi, Pi / 2, join, 10, 4);
    double after = Follow(0, Pi, Pi / 2, join + epsilon, 10, 4);
    near(at, Pi / 2);
    CHECK((std::abs((at - before) / epsilon - 10) < 1e-4));
    CHECK((std::abs((after - at) / epsilon - 10) < 1e-4));

    for (double invalid : {0., -1., std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()})
    {
        near(Follow(.25, Pi / 2, 0, invalid, 10, 4), .25);
        near(Follow(.25, Pi / 2, 0, .1, invalid, 4), .25);
    }
    near(Follow(.25, std::numeric_limits<double>::quiet_NaN(), 0, .1, 10, 4), .25);
    near(Follow(std::numeric_limits<double>::quiet_NaN(), .5, 0, .1, 10, 4), .5);
    CHECK((std::isfinite(Follow(0, Pi, std::numeric_limits<double>::quiet_NaN(), .1, 10, 4))));
    near(Follow(.25, .25, 0, 10, 10, 4), .25);
    near(Follow(Pi - .1, -Pi + .1, Pi - .1, 100, 10, 4), -Pi + .1);
}

} // namespace
