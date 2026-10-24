#include "gImpactAnimation.h"
#include "doctest.h"
#include <cmath>
#include <limits>

namespace
{
TEST_CASE("gImpactAnimation: continuous presentation and bounded geometry")
{
    using namespace gImpactAnimation;
    for (unsigned i = 0; i < FragmentCount; ++i)
        for (int frame = 0; frame <= 240; ++frame)
        {
            float t = frame / 120.f;
            auto a = Sample(i, t, 20), b = Sample(i, t, 20), fast = Sample(i, t, 60);
            CHECK((a.along == b.along && a.across == b.across && a.height == b.height));
            CHECK((a.spin == b.spin && a.fade == b.fade)); // repeated reflection/main sample
            CHECK((std::isfinite(a.along) && std::isfinite(a.height)));
            if (!a.Active())
                continue;
            CHECK((a.height >= .025f && a.height < 4));
            CHECK((a.fade <= 1 && a.sizeX > 0 && a.sizeY > 0 && a.sizeZ > 0));
            CHECK((a.sizeZ < a.sizeX));            // flat shards, not uniform cubes
            CHECK((fast.along >= a.along));        // stronger incoming momentum
            CHECK((fast.along - a.along <= 3.5f)); // impulse is bounded by drag
            CHECK((std::abs(a.across) < 3));
        }
    CHECK((Sample(0, 0, 20).Active()));   // initial vehicle panels exist at contact
    CHECK((!Sample(80, 0, 20).Active())); // smaller debris follows the contact flash
    CHECK((Sample(80, .15f, 20).Active()));
    CHECK((!Sample(0, Duration, 20).Active()));
    CHECK((!Sample(0, -.1f, 20).Active()));
    CHECK((!Sample(0, std::numeric_limits<float>::quiet_NaN(), 20).Active()));
    CHECK((!Sample(0, .2f, std::numeric_limits<float>::infinity()).Active()));
    CHECK((!Sample(180, 0, 20).Active())); // fine particles follow the shell breakup
    CHECK((Sample(180, .20f, 20).Active()));
    for (unsigned i = 0; i < RimFragmentCount; ++i)
    {
        auto initial = SampleRim(i, 0, 20, .7f);
        CHECK((initial.Active() && initial.pitch == 0 && initial.roll == 0));
        // The authored long-shell bike's rear axle is at -1.45.
        float axle = ((i / 12 >= 2 ? 1.55f : -1.45f) - 1.5f) * .7f;
        float x = initial.along - axle, z = initial.height - .72f * .7f;
        CHECK((std::abs(std::hypot(x, z) - .607f * .7f) < .0001f));
        CHECK((std::abs(std::abs(initial.across) - .446f * .7f) < .0001f));
        for (int frame = 0; frame < 240; ++frame)
        {
            float t = frame / 120.f;
            auto a = SampleRim(i, t, 20, .7f), b = SampleRim(i, t, 20, .7f), fast = SampleRim(i, t, 60, .7f);
            CHECK((a.along == b.along && a.height == b.height && a.pitch == b.pitch && a.roll == b.roll));
            CHECK((std::isfinite(a.along) && std::isfinite(a.height)));
            if (!a.Active())
                continue;
            CHECK((a.height >= .035f && a.height < 3 && a.fade <= 1));
            CHECK((fast.along >= a.along && fast.along - a.along <= 2.2f));
        }
    }
    CHECK((!SampleRim(RimFragmentCount, .2f, 20, .7f).Active()));
    CHECK((!SampleRim(0, .2f, 20, 0).Active()));
    CHECK((!SampleRim(0, -.01f, 20, .7f).Active()));
    CHECK((!SampleRim(0, .2f, 20, std::numeric_limits<float>::infinity()).Active()));
}

} // namespace
