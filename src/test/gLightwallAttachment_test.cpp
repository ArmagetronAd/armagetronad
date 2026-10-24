#include "gLightwallAttachment.h"
#include "gLightcycleMotion.h"
#include "gLightcycleProfile.h"
#include "doctest.h"
#include <vector>

namespace
{

using namespace gLightwallAttachment;
void near(Point a, Point b) { CHECK((std::hypot(a.x - b.x, a.y - b.y) < 1e-7)); }

TEST_CASE("gLightwallAttachment: continuous presentation and bounded geometry")
{
    // Straight runs attach at the fender, without pulling any settled vertices.
    auto straight = Curve::Join({0, 0}, {1, 0}, {1.4, 0}, {1, 0}, 4);
    near(straight.Sample(0), {0, 0});
    near(straight.Sample(1), {1.4, 0});
    double previous = -1;
    for (int i = 0; i <= 100; ++i)
    {
        auto p = straight.Sample(i / 100.);
        CHECK((p.x >= previous && p.x <= 1.4 && std::abs(p.y) < 1e-10));
        previous = p.x;
    }
    // The old avoidance route created a half-metre loop even when its two
    // endpoints coincided. A zero-length connection must be exactly empty,
    // and approaching/crossing zero must not expose a banked profile flap.
    for (double span : {0., 1e-7, 1e-5, .001, .01})
        for (int side : {-1, 1})
        {
            auto curve = Curve::Join({0, 0}, {1, 0}, {side * span, 0}, {1, 0}, .3);
            std::array<Point, 19> samples;
            for (int i = 0; i <= 18; ++i)
            {
                samples[i] = curve.Sample(i / 18.);
                CHECK((std::hypot(samples[i].x, samples[i].y) <= span + 1e-12));
            }
            auto sheet = UprightSheet(samples, 1.0656, .61875);
            for (auto const& column : sheet)
            {
                auto top = FittedVertex(column, {1, 0}, 1, .4, .44);
                CHECK((top.z == 1.0656));
                CHECK((std::hypot(top.x, top.y) <= span * 1.1 + 1e-12));
            }
        }

    // Moving the cut across a 90-degree corner cannot jump a control point.
    for (int sign : {-1, 1})
    {
        auto before = Curve::Join({-1e-7, 0}, {1, 0}, {1, sign * 2.}, {0, double(sign)}, 1e-7);
        auto after = Curve::Join({0, sign * 1e-7}, {0, double(sign)}, {1, sign * 2.}, {0, double(sign)}, 1e-7);
        for (int i = 0; i <= 100; ++i)
        {
            auto a = before.Sample(i / 100.), b = after.Sample(i / 100.);
            CHECK((std::hypot(a.x - b.x, a.y - b.y) < 1e-6));
        }
    }

    // A reversing emitter can cross the fixed wall's centreline. Changing the
    // preferred side must not flip the first handle across the wheel in one frame.
    for (double ahead : {0., .3, 1.0, 1.8})
    {
        auto left = Curve::Join({ahead, -1e-7}, {1, 0}, {0, 0}, {1, 0}, .2);
        auto right = Curve::Join({ahead, 1e-7}, {1, 0}, {0, 0}, {1, 0}, .2);
        for (int i = 0; i <= 100; ++i)
        {
            auto a = left.Sample(i / 100.), b = right.Sample(i / 100.);
            CHECK((std::hypot(a.x - b.x, a.y - b.y) < 1e-5));
        }
    }

    // Captured from the real SDL local-player stress run at 6.7-7.2 seconds.
    // These poses formerly triggered an outboard detour around the wheel.
    // That detour produced a moving loop. The connector must never reverse
    // along its chord; normal depth/tyre clipping handles hidden overlap.
    struct WheelCase
    {
        Point start, incoming;
        double room;
    };
    WheelCase wheelCases[] = {
        {{.0222586, .304806}, {.889632, .456678}, .107},
        {{.755564, .397928}, {-.835956, -.548796}, .299},
        {{1.052287, .553936}, {-.911463, -.411382}, .363},
        {{1.253438, .430313}, {-.968757, -.248013}, .382},
        {{1.430537, .463669}, {-.998320, -.057944}, .330},
        {{1.549287, .416934}, {-.993158, .116775}, .387}};
    for (auto const& test : wheelCases)
        for (int side : {-1, 1})
            for (double scale : {1., 1.65 / 1.4})
            {
                auto curve = Curve::Join({test.start.x * scale, side * test.start.y * scale},
                                         {test.incoming.x, side * test.incoming.y}, {0, 0}, {1, 0}, test.room * scale);
                near(curve.Sample(0), {test.start.x * scale, side * test.start.y * scale});
                near(curve.Sample(1), {0, 0});
                Point chord = curve.end - curve.start;
                double previous = -1e-9;
                for (int i = 0; i <= 200; ++i)
                {
                    Point p = curve.Sample(i / 200.) - curve.start;
                    double along = p.x * chord.x + p.y * chord.y;
                    CHECK((along >= previous - 1e-9));
                    previous = along;
                }
            }

    // Exercise the actual visual pose through isolated turns, rapid alternating
    // turns and successive same-direction turns, at different renderer rates.
    for (int fps : {30, 60, 120, 240})
        for (int mode = 0; mode < 3; ++mode)
        {
            struct Segment
            {
                double begin, end;
                Point start, dir;
            };
            std::vector<Segment> path;
            gLightcycleMotion::State pose;
            Point position{0, 0}, direction{1, 0};
            double distance = 0, yaw = 0, lastTurn = 0;
            constexpr double rear = (1.5 - gLightcycleProfile::RearExtent) * .825, speed = 20;
            path.push_back({0, 0, position, direction});
            pose.Update(0, yaw, 0, speed);
            double nextTurn = 1;
            int turns = 0;
            for (int frame = 1; frame < fps * 3; ++frame)
            {
                double now = double(frame) / fps, then = double(frame - 1) / fps;
                if (now >= nextTurn && (mode || turns == 0))
                {
                    double travel = (nextTurn - then) * speed;
                    position = position + direction * travel;
                    distance += travel;
                    path.back().end = distance;
                    yaw += gLightcycleMotion::Pi * .5 * (mode == 1 && turns % 2 ? -1 : 1);
                    direction = {std::cos(yaw), std::sin(yaw)};
                    lastTurn = nextTurn;
                    nextTurn += .075;
                    ++turns;
                    path.push_back({distance, distance, position, direction});
                    then = lastTurn;
                }
                position = position + direction * ((now - then) * speed);
                distance += (now - then) * speed;
                path.back().end = distance;
                pose.Update(now, yaw, lastTurn, speed);
                Point forward{std::cos(pose.Yaw()), std::sin(pose.Yaw())};
                Point emitter = position - forward * rear + Point{pose.VisualOffsetX(), pose.VisualOffsetY()};
                double cut = distance - Reach(rear);
                for (auto const& segment : path)
                {
                    if (cut < segment.begin || cut >= segment.end)
                        continue;
                    Point start = segment.start + segment.dir * (cut - segment.begin);
                    auto curve = Curve::Join(start, segment.dir, emitter, forward,
                                             std::min(cut - segment.begin, segment.end - cut));
                    std::array<Point, 19> samples;
                    for (unsigned i = 0; i < samples.size(); ++i)
                        samples[i] = curve.Sample(double(i) / (samples.size() - 1));
                    auto sheet = UprightSheet(samples, 1.22, .61875);
                    CHECK((std::abs(sheet.front().height - 1.22) < 1e-12));
                    CHECK((sheet.front().attachment == 0));
                    CHECK((std::abs(sheet.back().height - 1.22) < 1e-12));
                    double remaining = 0;
                    for (int i = 18; i >= 0; --i)
                    {
                        if (i < 18)
                            remaining += std::hypot(samples[i + 1].x - samples[i].x, samples[i + 1].y - samples[i].y);
                        auto bottom = UprightVertex(sheet[i], forward, 0);
                        auto top = UprightVertex(sheet[i], forward, 1);
                        // Rapid input and the bike's changing bank cannot lift the
                        // floor edge, tip the sheet, or move the settled-height edge.
                        // Only the short emitter collar fits the banking rear stripe.
                        // Neither bank direction may move its floor or change height.
                        for (double slope : {-.44, 0., .44})
                        {
                            auto foot = FittedVertex(sheet[i], forward, 0, .3, slope);
                            auto fitted = FittedVertex(sheet[i], forward, 1, .3, slope);
                            CHECK((foot.z == 0 && fitted.z == top.z));
                            auto upright = UprightVertex(sheet[i], forward, 1, .3);
                            CHECK((std::hypot(fitted.x - upright.x, fitted.y - upright.y) <= top.z * .44 + 1e-9));
                            if (sheet[i].attachment == 0)
                                near({fitted.x, fitted.y}, {upright.x, upright.y});
                            if (i == 18)
                            {
                                double side = -(fitted.x - upright.x) * forward.y + (fitted.y - upright.y) * forward.x;
                                CHECK((std::abs(side + top.z * slope * sheet[i].attachment) < 1e-9));
                            }
                        }
                        CHECK((bottom.z == 0));
                        CHECK((top.x == bottom.x && top.y == bottom.y));
                        CHECK((std::abs(top.z - 1.22) < 1e-12));
                        if (remaining >= .61875)
                        {
                            CHECK((sheet[i].attachment == 0));
                            CHECK((std::abs(top.z - 1.22) < 1e-12));
                            auto fitted = UprightVertex(sheet[i], forward, 1, .3);
                            CHECK((fitted.x == top.x && fitted.y == top.y));
                        }
                    }
                    // The actual surface rows must also be monotone. The former
                    // per-vertex profile offsets folded the top edge back during
                    // fast turns, despite a stable centreline and constant height.
                    for (double height : {0., .53, 1.0656})
                        for (double profile : {0., .15, .40})
                            for (double slope : {-.44, 0., .44})
                            {
                                auto row = ProfileCurve(curve, forward, height, profile, slope, sheet.back().attachment);
                                Point chord = row.end - row.start;
                                double previous = -1e-9;
                                for (int i = 0; i <= 18; ++i)
                                {
                                    Point relative = row.Sample(i / 18.) - row.start;
                                    double along = relative.x * chord.x + relative.y * chord.y;
                                    CHECK((along >= previous - 1e-8));
                                    previous = along;
                                }
                            }
                    near(curve.Sample(0), start);
                    near(curve.Sample(1), emitter);
                    auto tangent = curve.Tangent(1);
                    CHECK((tangent.x * forward.x + tangent.y * forward.y >= 0));
                    CHECK((std::abs(tangent.x * forward.y - tangent.y * forward.x) < 1e-7));
                    // The complete connector is monotone, even when the emitter
                    // briefly faces back towards the previous physical wall.
                    Point chord = emitter - start;
                    double previousAlong = -1e-9;
                    for (int i = 0; i <= 100; ++i)
                    {
                        Point relative = curve.Sample(i / 100.) - start;
                        double along = relative.x * chord.x + relative.y * chord.y;
                        CHECK((along >= previousAlong - 1e-8));
                        previousAlong = along;
                    }
                    for (int i = 0; i <= 100; ++i)
                    {
                        auto p = curve.Sample(i / 100.);
                        CHECK((std::isfinite(p.x) && std::isfinite(p.y)));
                        CHECK((std::hypot(p.x - position.x, p.y - position.y) < Reach(rear) + 2));
                        CHECK((curve.SpillWidth(i / 100.) >= 0 && curve.SpillWidth(i / 100.) <= 1));
                    }
                }
            }
        }
    // A collapsed/very short connector remains finite and joins the full wall.
    std::array<Point, 3> shortPath = {{{0, 0}, {.01, 0}, {.02, 0}}};
    auto shortSheet = UprightSheet(shortPath, 1.22, .6);
    CHECK((shortSheet.front().height == 1.22 && shortSheet[1].height == 1.22));
    auto collapsed = UprightSheet(std::array<Point, 3>{}, 1.22, .6);
    for (auto const& column : collapsed)
        CHECK((column.height == 1.22 && column.attachment == 0));
}

} // namespace
