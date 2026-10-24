// Geometry for the short visual connection between a fixed wall and its emitter.
// Licensed under the project's GNU GPL version 2 or later.
#ifndef ARMAGETRON_LIGHTWALL_ATTACHMENT_H
#define ARMAGETRON_LIGHTWALL_ATTACHMENT_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>

namespace gLightwallAttachment
{
struct Point
{
    double x, y;
    Point operator+(Point b) const { return {x + b.x, y + b.y}; }
    Point operator-(Point b) const { return {x - b.x, y - b.y}; }
    Point operator*(double s) const { return {x * s, y * s}; }
};

struct Curve
{
    Point start, first, last, end;

    Point Sample(double t) const
    {
        t = std::max(0., std::min(1., t));
        double s = 1 - t;
        return start * (s * s * s) + first * (3 * s * s * t) + last * (3 * s * t * t) + end * (t * t * t);
    }

    Point Tangent(double t) const
    {
        double s = 1 - t;
        return (first - start) * (3 * s * s) + (last - first) * (6 * s * t) + (end - last) * (3 * t * t);
    }

    double SpillWidth(double t) const
    {
        Point tangent = Tangent(t);
        Point bend = (last - first * 2 + start) * (6 * (1 - t)) + (end - last * 2 + first) * (6 * t);
        double speed = std::hypot(tangent.x, tangent.y);
        double cross = std::abs(tangent.x * bend.y - tangent.y * bend.x);
        // Offset ribbons fold over themselves if wider than the local bend.
        return cross > 1e-9 ? std::min(1., .55 * speed * speed * speed / cross) : 1.;
    }

    static Curve Join(Point start, Point incoming, Point end, Point forward, double cornerRoom)
    {
        Point chord = end - start;
        double span = std::hypot(chord.x, chord.y);
        // Keep every control point ordered along the join-to-emitter chord.
        // Routing a reversed emitter around the tyre made this connector double
        // back, then flip its loop to the other side during alternating turns.
        // The opaque bike and the tyre clip occlude any newborn overlap instead.
        // A tangent that faces away from the chord shrinks continuously to zero.
        double entering = chord.x * incoming.x + chord.y * incoming.y;
        double leaving = chord.x * forward.x + chord.y * forward.y;
        double firstLength = std::min({span * .35, std::max(0., cornerRoom), std::max(0., entering) * .45});
        double lastLength = std::min(span * .35, std::max(0., leaving) * .45);
        return {start, start + incoming * firstLength, end - forward * lastLength, end};
    }
};

struct Column
{
    Point base;
    double height;
    double attachment;
};

// Every column has the same height from birth. Only a short collar at the
// emitter fits the rear housing in the horizontal plane. A lower tip that grows
// to full height made the ribbon rise and fall when a turn reshaped the collar.
// World-distance weighting also avoids stretching this collar across an entire
// connector when a rapid turn changes the connector's shape or length.
template <std::size_t N>
inline std::array<Column, N> UprightSheet(std::array<Point, N> const& path,
                                          double height, double collarLength)
{
    static_assert(N >= 2, "A ribbon requires at least two columns");
    std::array<double, N> remaining{};
    for (std::size_t i = N - 1; i > 0; --i)
        remaining[i - 1] = remaining[i] + std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
    // Even a short connector must meet the full-height settled wall at its start.
    double collar = std::min(std::max(0., collarLength), remaining[0] * .5);
    // A collapsing connection must also collapse its surface-fitting offsets.
    // Full bank/profile offsets on an arbitrarily short span created a new flap
    // even when the centreline itself had no loop.
    double fit = collarLength > 1e-9 ? std::max(0., std::min(1., remaining[0] / collarLength)) : 0;
    fit = fit * fit * (3 - 2 * fit);
    std::array<Column, N> result;
    for (std::size_t i = 0; i < N; ++i)
    {
        double blend = collar > 1e-9 ? std::max(0., std::min(1., 1 - remaining[i] / collar)) : 0;
        blend = blend * blend * (3 - 2 * blend);
        result[i] = {path[i], height, blend * fit};
    }
    return result;
}

struct Vertex
{
    double x, y, z;
};
inline Vertex UprightVertex(Column const& column, Point forward, double vertical,
                            double rearProfileOffset = 0)
{
    Point point = column.base + forward * (rearProfileOffset * column.attachment);
    return {point.x, point.y, column.height * vertical};
}

// Only the short collar follows the rear stripe as the motorcycle banks. Keep
// its height and floor edge fixed; applying roll to the whole ribbon made it flap.
inline Vertex FittedVertex(Column const& column, Point forward, double vertical,
                           double rearProfileOffset, double bankSlope)
{
    Vertex vertex = UprightVertex(column, forward, vertical, rearProfileOffset);
    double side = vertex.z * bankSlope * column.attachment;
    vertex.x += forward.y * side;
    vertex.y -= forward.x * side;
    return vertex;
}

// Fit each horizontal row as a bounded curve in its own right. Adding a
// profile offset after sampling the centreline could make its upper rows fold
// even when the centreline was monotone. All rows retain the same fixed join.
inline Curve ProfileCurve(Curve const& centre, Point forward, double height,
                          double profileOffset, double bankSlope, double fit)
{
    Point handle = centre.first - centre.start;
    double length = std::hypot(handle.x, handle.y);
    Point incoming = length > 1e-9 ? handle * (1 / length) : Point{0, 0};
    Point end = centre.end + forward * (profileOffset * fit) + Point{forward.y, -forward.x} * (height * bankSlope * fit);
    return Curve::Join(centre.start, incoming, end, forward, length);
}

// Extra distance behind the rear fender where the fixed wall becomes a curve.
// It stays inside the existing five-metre uncached wall window at normal scale.
inline double Reach(double rearDistance) { return std::min(4.9, rearDistance + 1.4); }
} // namespace gLightwallAttachment
#endif
