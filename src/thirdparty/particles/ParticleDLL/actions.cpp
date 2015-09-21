// actions.cpp
//
// Copyright 1997-2006 by David K. McAllister
//
// I used code Copyright 1997 by Jonathan P. Leech
// as an example in implenting this.
//
// This file implements the dynamics of particle actions.

#include "actions.h"
#include "ParticleState.h"

#include <algorithm>
// For dumping errors
#include <sstream>

// Compute the inverse matrix of the plane basis.
static inline void NewBasis(const pVec &u, const pVec &v, pVec &s1, pVec &s2)
{
    pVec w = Cross(u, v);

    float det = 1.0f / (w.z()*u.x()*v.y() - w.z()*u.y()*v.x() - u.z()*w.x()*v.y() - u.x()*v.z()*w.y() + v.z()*w.x()*u.y() + u.z()*v.x()*w.y());

    s1 = pVec((v.y()*w.z() - v.z()*w.y()), (v.z()*w.x() - v.x()*w.z()), (v.x()*w.y() - v.y()*w.x()));
    s1 *= det;
    s2 = pVec((u.y()*w.z() - u.z()*w.y()), (u.z()*w.x() - u.x()*w.z()), (u.x()*w.y() - u.y()*w.x()));
    s2 *= -det;
}

void PAAvoid::Exec(const PDTriangle &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;

    const pVec &u = dom.u;
    const pVec &v = dom.v;

    // The normalized bases are needed inside the loop.
    const pVec &un = dom.uNrm;
    const pVec &vn = dom.vNrm;

    // f is the third (non-basis) triangle edge.
    pVec f = v - u;
    pVec fn = f;
    fn.normalize();

    // Compute the inverse matrix of the plane basis.
    pVec s1, s2;
    NewBasis(u, v, s1, s2);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt * look_ahead;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv; // Time steps before hit

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        // Dot product with basis vectors of old frame
        // in terms of new frame gives position in uv frame.
        float upos = offset * s1;
        float vpos = offset * s2;

        // Did it cross plane outside triangle?
        if(upos < 0 || vpos < 0 || (upos + vpos) > 1)
            continue;

        // A hit! A most palpable hit!
        // Compute distance to the three edges.
        pVec uofs = (un * (un * offset)) - offset;
        float udistSqr = uofs.length2();
        pVec vofs = (vn * (vn * offset)) - offset;
        float vdistSqr = vofs.length2();

        pVec foffset = offset - u;
        pVec fofs = (fn * (fn * foffset)) - foffset;
        float fdistSqr = fofs.length2();

        // S is the safety vector toward the closest point on boundary.
        pVec S;
        if(udistSqr <= vdistSqr && udistSqr <= fdistSqr) S = uofs;
        else if(vdistSqr <= fdistSqr) S = vofs;
        else S = fofs;

        // Blend S with m.vel.
        S.normalize();

        float vm = m.vel.length();
        pVec Vn = m.vel / vm;

        pVec dir = (S * (magdt / (fsqr(t)+epsilon))) + Vn;
        m.vel = dir * (vm / dir.length()); // Speed of m.vel, but in direction dir.
    }
}

void PAAvoid::Exec(const PDRectangle &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;

    const pVec &u = dom.u;
    const pVec &v = dom.v;

    // The normalized bases are needed inside the loop.
    const pVec &un = dom.uNrm;
    const pVec &vn = dom.vNrm;

    // Compute the inverse matrix of the plane basis.
    pVec s1, s2;
    NewBasis(u, v, s1, s2);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt * look_ahead;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv;

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        // Dot product with basis vectors of old frame
        // in terms of new frame gives position in uv frame.
        float upos = offset * s1;
        float vpos = offset * s2;

        // Did it cross plane outside rectangle?
        if(upos < 0 || vpos < 0 || upos > 1 || vpos > 1)
            continue;

        // A hit! A most palpable hit!
        // Compute distance to the three edges.
        pVec uofs = (un * (un * offset)) - offset;
        float udistSqr = uofs.length2();
        pVec vofs = (vn * (vn * offset)) - offset;
        float vdistSqr = vofs.length2();

        pVec foffset = (u + v) - offset;
        pVec fofs = (un * (un * foffset)) - foffset;
        float fdistSqr = fofs.length2();
        pVec gofs = (un * (un * foffset)) - foffset;
        float gdistSqr = gofs.length2();

        pVec S;
        if(udistSqr <= vdistSqr && udistSqr <= fdistSqr
                && udistSqr <= gdistSqr) S = uofs;
        else if(vdistSqr <= fdistSqr && vdistSqr <= gdistSqr) S = vofs;
        else if(fdistSqr <= gdistSqr) S = fofs;
        else S = gofs;

        // Blend S with m.vel.
        S.normalize();

        float vm = m.vel.length();
        pVec Vn = m.vel / vm;

        pVec dir = (S * (magdt / (fsqr(t)+epsilon))) + Vn;
        m.vel = dir * (vm / dir.length()); // Speed of m.vel, but in direction dir.
    }
}

void PAAvoid::Exec(const PDPlane &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt * look_ahead;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv;

        // Blend S with m.vel.
        const pVec &S = dom.nrm;

        float vm = m.vel.length();
        pVec Vn = m.vel / vm;

        pVec dir = (S * (magdt / (fsqr(t)+epsilon))) + Vn;
        m.vel = dir * (vm / dir.length()); // Speed of m.vel, but in direction dir.
    }
}

// Only works for points on the OUTSIDE of the sphere. Ignores inner radius.
void PAAvoid::Exec(const PDSphere &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // First do a ray-sphere intersection test and see if it's soon enough.
        // Can I do this faster without t?
        float vm = m.vel.length();
        pVec Vn = m.vel / vm;

        pVec L = dom.ctr - m.pos;
        float v = L * Vn;

        float disc = dom.radOutSqr - (L * L) + fsqr(v);
        if(disc < 0)
            continue; // I'm not heading toward it.

        // Compute length for second rejection test.
        float t = v - sqrtf(disc);
        if(t < 0 || t > (vm * look_ahead))
            continue;

        // Get a vector to safety.
        pVec C = Cross(Vn, L);
        C.normalize();
        pVec S = Cross(Vn, C);

        pVec dir = (S * (magdt / (fsqr(t)+epsilon))) + Vn;
        m.vel = dir * (vm / dir.length()); // Speed of m.vel, but in direction dir.
    }
}

void PAAvoid::Exec(const PDDisc &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt * look_ahead;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv;

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        float radSqr = offset.length2();

        // Are we going to hit the disc ring? If so, always turn to the OUTSIDE of the ring.
        if(radSqr < dom.radInSqr || radSqr > dom.radOutSqr)
            continue;

        // Blend S with m.vel.
        pVec S = offset;

        float vm = m.vel.length();
        pVec Vn = m.vel / vm;

        pVec dir = (S * (magdt / (fsqr(t)+epsilon))) + Vn;
        m.vel = dir * (vm / dir.length()); // Speed of m.vel, but in direction dir.
    }
}

void PAAvoid::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    if(typeid(*position) == typeid(PDTriangle)) {
        Exec(*dynamic_cast<const PDTriangle *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDDisc)) {
        Exec(*dynamic_cast<const PDDisc *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDPlane)) {
        Exec(*dynamic_cast<const PDPlane *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDRectangle)) {
        Exec(*dynamic_cast<const PDRectangle *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDSphere)) {
        Exec(*dynamic_cast<const PDSphere *>(position), group, ibegin, iend);
    } else {
        std::string err = std::string("pAvoid not implemented for domain ") + std::string(typeid(*position).name());
        _GetPState().SetError(PERR_NOT_IMPLEMENTED, err);
    }
}

void PABounce::Exec(const PDTriangle &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    pVec s1, s2;
    NewBasis(dom.u, dom.v, s1, s2);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv; // Time steps before hit

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        // Dot product with basis vectors of old frame
        // in terms of new frame gives position in uv frame.
        float upos = offset * s1;
        float vpos = offset * s2;

        // Did it cross plane outside triangle?
        if(upos < 0 || vpos < 0 || (upos + vpos) > 1)
            continue;

        // A hit! A most palpable hit!
        // Compute tangential and normal components of velocity
        pVec vn = dom.nrm * nv; // Normal Vn = (V.N)N
        pVec vt = m.vel - vn;   // Tangent Vt = V - Vn

        // Compute new velocity heading out:
        // Don't apply friction if tangential velocity < cutoff
        if(vt.length2() <= cutoffSqr)
            m.vel = vt - vn * resilience;
        else
            m.vel = vt * oneMinusFriction - vn * resilience;
    }
}

void PABounce::Exec(const PDRectangle &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    pVec s1, s2;
    NewBasis(dom.u, dom.v, s1, s2);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and pnext positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv; // Time steps before hit

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        // Dot product with basis vectors of old frame
        // in terms of new frame gives position in uv frame.
        float upos = offset * s1;
        float vpos = offset * s2;

        // Did it cross plane outside rectangle?
        if(upos < 0 || upos > 1 || vpos < 0 || vpos > 1)
            continue;

        // A hit! A most palpable hit!
        // Compute tangential and normal components of velocity
        pVec vn = dom.nrm * nv; // Normal Vn = (V.N)N
        pVec vt = m.vel - vn;   // Tangent Vt = V - Vn

        // Compute new velocity heading out:
        // Don't apply friction if tangential velocity < cutoff
        if(vt.length2() <= cutoffSqr)
            m.vel = vt - vn * resilience;
        else
            m.vel = vt * oneMinusFriction - vn * resilience;
    }
}

void PABounce::Exec(const PDPlane &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv; // Time steps before hit

        // A hit! A most palpable hit!
        // Compute tangential and normal components of velocity
        pVec vn = dom.nrm * nv; // Normal Vn = (V.N)N
        pVec vt = m.vel - vn;   // Tangent Vt = V - Vn

        // Compute new velocity heading out:
        // Don't apply friction if tangential velocity < cutoff
        if(vt.length2() <= cutoffSqr)
            m.vel = vt - vn * resilience;
        else
            m.vel = vt * oneMinusFriction - vn * resilience;
    }
}

void PABounce::Exec(const PDSphere &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(dom.radIn == 0.0f, "Bouncing doesn't work on thick shells. radIn must be 0.");

    float dtinv = 1.0f / dt;

    // Bounce particles off the inside or outside of the sphere
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's next position is on the opposite side of the domain. If so, bounce it.
        pVec pnext = m.pos + m.vel * dt;

        if(dom.Within(m.pos)) {
            // We are bouncing off the inside of the sphere.
            if(dom.Within(pnext))
                // Still inside. Do nothing.
                continue;

            // Trying to go outside. Bounce back in.

            // Inward-pointing normal to surface. This isn't computed quite right;
            // should extrapolate particle position to surface.
            pVec n(dom.ctr - m.pos);
            n.normalize();

            // Compute tangential and normal components of velocity
            float nmag = m.vel * n;

            pVec vn = n * nmag;   // Velocity in Normal dir  Vn = (V.N)N
            pVec vt = m.vel - vn; // Velocity in Tangent dir Vt = V - Vn

            // Reverse normal component of velocity
            if(nmag < 0) vn = -vn; // Don't reverse if it's already heading inward

            // Compute new velocity heading out:
            // Don't apply friction if tangential velocity < cutoff
            float tanscale = (vt.length2() <= cutoffSqr) ? 1.0f : oneMinusFriction;
            m.vel = vt * tanscale + vn * resilience;

            // Now see where the point will end up. Make sure we fixed it to stay inside.
            pVec pthree = m.pos + m.vel * dt;
            if(dom.Within(pthree)) {
                // Still inside. We're good.
                continue;
            } else {
                // Since the tangent plane is outside the sphere, reflecting the velocity vector about it won't necessarily bring it inside the sphere.
                pVec toctr = dom.ctr - pthree;
                float dist = toctr.length();
                pVec pwish = dom.ctr - toctr * (0.999f * dom.radOut / dist); // pwish is a point just inside the sphere
                m.vel = (pwish - m.pos) * dtinv; // Compute a velocity to get us to pwish.
            }
        } else {
            // We are bouncing off the outside of the sphere.
            if(!dom.Within(pnext))
                continue;

            // Trying to go inside. Bounce back out.

            // Outward-pointing normal to surface. This isn't computed quite right;
            // should extrapolate particle position to surface.
            pVec n = m.pos - dom.ctr;
            n.normalize();

            // Compute tangential and normal components of velocity
            float nmag = m.vel * n;

            pVec vn = n * nmag;   // Velocity in Normal dir  Vn = (V.N)N
            pVec vt = m.vel - vn; // Velocity in Tangent dir Vt = V - Vn

            // Reverse normal component of velocity if it points in
            if(nmag < 0)
                vn = -vn;

            // Compute new velocity heading out:
            // Don't apply friction if tangential velocity < cutoff
            float tanscale = (vt.length2() <= cutoffSqr) ? 1.0f : oneMinusFriction;
            m.vel = vt * tanscale + vn * resilience;
        }
    }
}

void PABounce::Exec(const PDDisc &dom, ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // See if particle's current and look_ahead positions cross plane.
        // If not, couldn't hit, so keep going.
        pVec pnext = m.pos + m.vel * dt;

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        float distold = m.pos * dom.nrm + dom.D;
        float distnew = pnext * dom.nrm + dom.D;

        if(pSameSign(distold, distnew))
            continue;

        float nv = dom.nrm * m.vel;
        float t = -distold / nv; // Time steps before hit

        pVec phit = m.pos + m.vel * t; // Actual intersection point
        pVec offset = phit - dom.p; // Offset from origin in plane

        float radSqr = offset.length2();

        // Are we going to hit the disc ring? If so, always turn to the OUTSIDE of the ring.
        if(radSqr < dom.radInSqr || radSqr > dom.radOutSqr)
            continue;

        // A hit! A most palpable hit!
        // Compute tangential and normal components of velocity
        pVec vn = dom.nrm * nv; // Normal Vn = (V.N)N
        pVec vt = m.vel - vn;   // Tangent Vt = V - Vn

        // Compute new velocity heading out:
        // Don't apply friction if tangential velocity < cutoff
        if(vt.length2() <= cutoffSqr)
            m.vel = vt - vn * resilience;
        else
            m.vel = vt * oneMinusFriction - vn * resilience;
    }
}

void PABounce::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    if(typeid(*position) == typeid(PDTriangle)) {
        Exec(*dynamic_cast<const PDTriangle *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDDisc)) {
        Exec(*dynamic_cast<const PDDisc *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDPlane)) {
        Exec(*dynamic_cast<const PDPlane *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDRectangle)) {
        Exec(*dynamic_cast<const PDRectangle *>(position), group, ibegin, iend);
    } else if(typeid(*position) == typeid(PDSphere)) {
        Exec(*dynamic_cast<const PDSphere *>(position), group, ibegin, iend);
    } else {
        std::string err = std::string("pBounce not implemented for domain ") + std::string(typeid(*position).name());
        _GetPState().SetError(PERR_NOT_IMPLEMENTED, err);
    }
}

// An action list within an action list
void PACallActionList::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");
    pCallActionList(action_list_num);
}

// Set the secondary position and velocity from current.
void PACopyVertexB::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    if(copy_pos && copy_vel) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);
            m.posB = m.pos;
            m.velB = m.vel;
        }
    } else if(copy_pos) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);
            m.posB = m.pos;
        }
    } else if(copy_vel) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);
            m.velB = m.vel;
            m.rvelB = m.rvel;
        }
    }
}

// Dampen velocities
void PADamping::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    // This is important if dt is != 1.
    pVec one(1,1,1);
    pVec scale(one - ((one - damping) * dt));

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        float vSqr = m.vel.length2();

        if(vSqr >= vlowSqr && vSqr <= vhighSqr) {
            m.vel = CompMult(m.vel, scale);
        }
    }
}

// Dampen rotational velocities
void PARotDamping::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    // This is important if dt is != 1.
    pVec one(1,1,1);
    pVec scale(one - ((one - damping) * dt));

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        float vSqr = m.rvel.length2();

        if(vSqr >= vlowSqr && vSqr <= vhighSqr) {
            m.rvel = CompMult(m.rvel, scale);
        }
    }
}

// Exert force on each particle away from explosion center
void PAExplosion::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float radius = velocity * age;
    float magdt = magnitude * dt;
    float oneOverSigma = 1.0f / stdev;
    float inexp = -0.5f*fsqr(oneOverSigma);
    float outexp = P_ONEOVERSQRT2PI * oneOverSigma;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // Figure direction to particle.
        pVec dir(m.pos - center);
        float distSqr = dir.length2();
        float dist = sqrtf(distSqr);
        float DistFromWaveSqr = fsqr(radius - dist);

        float Gd = exp(DistFromWaveSqr * inexp) * outexp;
        pVec amount = dir * (Gd * magdt / (dist * (distSqr + epsilon)));

        m.vel += amount;
    }

    age += dt;
}

// Follow the next particle in the list
void PAFollow::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    float magdt = magnitude * dt;
    float max_radiusSqr = fsqr(max_radius);

    if (group.size() < 2)
        return;

    ParticleList::iterator end = iend;
    end--;

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != end; it++) {
            Particle &m = (*it);
            ParticleList::iterator next = it;
            next++;

            // Accelerate toward the particle after me in the list.
            pVec tohim((*next).pos - m.pos); // tohim = p1 - p0
            float tohimlenSqr = tohim.length2();

            if(tohimlenSqr < max_radiusSqr) {
                // Compute force exerted between the two bodies
                m.vel += tohim * (magdt / (sqrtf(tohimlenSqr) * (tohimlenSqr + epsilon)));
            }
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != end; it++) {
            Particle &m = (*it);
            ParticleList::iterator next = it;
            next++;

            // Accelerate toward the particle after me in the list.
            pVec tohim((*next).pos - m.pos); // tohim = p1 - p0
            float tohimlenSqr = tohim.length2();

            // Compute force exerted between the two bodies
            m.vel += tohim * (magdt / (sqrtf(tohimlenSqr) * (tohimlenSqr + epsilon)));
        }
    }
}

// Inter-particle gravitation
void PAGravitate::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    float magdt = magnitude * dt;
    float max_radiusSqr = max_radius * max_radius;

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            ParticleList::iterator j = it;
            j++;

            // Add interactions with other particles
            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                if(tohimlenSqr < max_radiusSqr) {
                    // Compute force exerted between the two bodies
                    pVec acc(tohim * (magdt / (sqrtf(tohimlenSqr) * (tohimlenSqr + epsilon))));

                    m.vel += acc;
                    mj.vel -= acc;
                }
            }
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Add interactions with other particles
            ParticleList::iterator j = it;
            ++j;

            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                // Compute force exerted between the two bodies
                pVec acc(tohim * (magdt / (sqrtf(tohimlenSqr) * (tohimlenSqr + epsilon))));

                m.vel += acc;
                mj.vel -= acc;
            }
        }
    }
}

// Acceleration in a constant direction
void PAGravity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    pVec ddir(direction * dt);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        // Step velocity with acceleration
        m.vel += ddir;
    }
}

// For particles in the domain of influence, accelerate them with a domain.
void PAJet::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        if(dom->Within(m.pos)) {
            pVec accel = acc->Generate();

            // Step velocity with acceleration
            m.vel += accel * dt;
        }
    }
}

// Get rid of older particles
void PAKillOld::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    // Must traverse list carefully so Remove will work
    for (ParticleList::iterator it = ibegin; it != group.end(); ) {
        Particle &m = (*it);

        if(!((m.age < age_limit) ^ kill_less_than))
            group.Remove(it);
        else
            it++;
    }
}

// Match velocity to near neighbors
void PAMatchVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    float magdt = magnitude * dt;
    float max_radiusSqr = max_radius * max_radius;

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Add interactions with other particles
            ParticleList::iterator j = it;
            j++;

            // Add interactions with other particles
            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                if(tohimlenSqr < max_radiusSqr) {
                    // Compute force exerted between the two bodies
                    pVec acc(mj.vel * (magdt / (tohimlenSqr + epsilon)));

                    m.vel += acc;
                    mj.vel -= acc;
                }
            }
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Add interactions with other particles
            ParticleList::iterator j = it;
            j++;

            // Add interactions with other particles
            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                // Compute force exerted between the two bodies
                pVec acc(mj.vel * (magdt / (tohimlenSqr + epsilon)));

                m.vel += acc;
                mj.vel -= acc;
            }
        }
    }
}

// Match Rotational velocity to near neighbors
void PAMatchRotVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    float magdt = magnitude * dt;
    float max_radiusSqr = max_radius * max_radius;

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Add interactions with other particles
            ParticleList::iterator j = it;
            j++;

            // Add interactions with other particles
            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                if(tohimlenSqr < max_radiusSqr) {
                    // Compute force exerted between the two bodies
                    pVec acc(mj.rvel * (magdt / (tohimlenSqr + epsilon)));

                    m.rvel += acc;
                    mj.rvel -= acc;
                }
            }
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Add interactions with other particles
            ParticleList::iterator j = it;
            j++;

            // Add interactions with other particles
            for(; j != iend; ++j) {
                Particle &mj = (*j);

                pVec tohim(mj.pos - m.pos); // tohim = p1 - p0
                float tohimlenSqr = tohim.length2();

                // Compute force exerted between the two bodies
                pVec acc(mj.rvel * (magdt / (tohimlenSqr + epsilon)));

                m.rvel += acc;
                mj.rvel -= acc;
            }
        }
    }
}

void PAMove::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    // Step particle positions forward by dt, and age the particles.
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        m.age += dt;
        m.pos += m.vel * dt;
        m.up += m.rvel * dt;
    }
}

// Accelerate particles towards a line
void PAOrbitLine::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;
    float max_radiusSqr = fsqr(max_radius);

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Figure direction to particle from base of line.
            pVec f = m.pos - p;

            // Projection of particle onto line
            pVec w = axis * (f * axis);

            // Direction from particle to nearest point on line.
            pVec into = w - f;

            // Distance to line (force drops as 1/r^2, normalize by 1/r)
            // Soften by epsilon to avoid tight encounters to infinity
            float rSqr = into.length2();

            if(rSqr < max_radiusSqr)
                // Step velocity with acceleration
                m.vel += into * (magdt / (sqrtf(rSqr) * (rSqr + epsilon)));
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Figure direction to particle from base of line.
            pVec f = m.pos - p;

            // Projection of particle onto line
            pVec w = axis * (f * axis);

            // Direction from particle to nearest point on line.
            pVec into = w - f;

            // Distance to line (force drops as 1/r^2, normalize by 1/r)
            // Soften by epsilon to avoid tight encounters to infinity
            float rSqr = into.length2();

            // Step velocity with acceleration
            m.vel += into * (magdt / (sqrtf(rSqr) * (rSqr + epsilon)));
        }
    }
}

// Accelerate particles towards a point
void PAOrbitPoint::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;
    float max_radiusSqr = max_radius * max_radius;

    if(max_radiusSqr < P_MAXFLOAT) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Figure direction to particle.
            pVec dir(center - m.pos);

            // Distance to gravity well (force drops as 1/r^2, normalize by 1/r)
            // Soften by epsilon to avoid tight encounters to infinity
            float rSqr = dir.length2();

            // Step velocity with acceleration
            if(rSqr < max_radiusSqr)
                m.vel += dir * (magdt / (sqrtf(rSqr) * (rSqr + epsilon)));
        }
    } else {
        // If not using radius cutoff, avoid the if().
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Figure direction to particle.
            pVec dir(center - m.pos);

            // Distance to gravity well (force drops as 1/r^2, normalize by 1/r)
            // Soften by epsilon to avoid tight encounters to infinity
            float rSqr = dir.length2();

            // Step velocity with acceleration
            m.vel += dir * (magdt / (sqrtf(rSqr) * (rSqr + epsilon)));
        }
    }
}

// Accelerate in random direction each time step
void PARandomAccel::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        pVec accel = gen_acc->Generate();

        // dt will affect this by making a higher probability of
        // being near the original velocity after unit time. Smaller
        // dt approach a normal distribution instead of a square wave.
        m.vel += accel * dt;
    }
}

// Immediately displace position randomly
void PARandomDisplace::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        pVec disp = gen_disp->Generate();

        // dt will affect this by making a higher probability of
        // being near the original position after unit time. Smaller
        // dt approach a normal distribution instead of a square wave.
        m.pos += disp * dt;
    }
}

// Immediately assign a random velocity
void PARandomVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        pVec velocity = gen_vel->Generate();

        // Shouldn't multiply by dt because velocities are
        // invariant of dt. How should dt affect this?
        m.vel = velocity;
    }
}

// Immediately assign a random rotational velocity
void PARandomRotVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        pVec velocity = gen_vel->Generate();

        // Shouldn't multiply by dt because velocities are
        // invariant of dt. How should dt affect this?
        m.rvel = velocity;
    }
}

#if 0
// Produce coefficients of a velocity function v(t)=at^2 + bt + c
// satisfying initial x(0)=x0,v(0)=v0 and desired x(t)=x1,v(t)=v1,
// where x = x(0) + integral(v(T),0,t)
static inline void _pconstrain(float x0, float v0, float x1, float v1,
                               float t, float *a, float *b, float *c)
{
    *c = v0;
    *b = 2 * (-t*v1 - 2*t*v0 + 3*x1 - 3*x0) / (t * t);
    *a = 3 * (t*v1 + t*v0 - 2*x1 + 2*x0) / (t * t * t);
}

// Solve for a desired-behavior velocity function in each axis
// _pconstrain(m.pos.x(), m.vel.x(), m.posB.x(), 0., timeLeft, &a, &b, &c);

// Figure new velocity at next timestep
// m.vel.x() = a * dtSqr + b * dt + c;
#endif

// Figure new velocity at next timestep
static inline void Restore(pVec &vel, const pVec &posB, const pVec &pos, const float t,
                           const float dtSqr, const float ttInv6dt, const float tttInv3dtSqr)
{
    pVec b = (vel*-0.6667f*t + posB - pos) * ttInv6dt;
    pVec a = (vel*t - posB - posB + pos + pos) * tttInv3dtSqr;
    vel += a + b;
}

// Over time, restore particles to initial positions
void PARestore::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    if(time_left <= 0) {
        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            // Already constrained; keep it there.
            m.pos = m.posB;
            m.vel = pVec(0.0f,0.0f,0.0f);
            m.rvel = pVec(0.0f,0.0f,0.0f);
            m.up = m.upB;
        }
    } else {
        float t = time_left;
        float dtSqr = fsqr(dt);
        float ttInv6dt = dt * 6.0f / fsqr(t);
        float tttInv3dtSqr = dtSqr * 3.0f / (t * t * t);

        for (ParticleList::iterator it = ibegin; it != iend; it++) {
            Particle &m = (*it);

            if (restore_velocity)
                Restore(m.vel, m.posB, m.pos, t, dtSqr, ttInv6dt, tttInv3dtSqr);
            if (restore_rvelocity)
                Restore(m.rvel, m.upB, m.up, t, dtSqr, ttInv6dt, tttInv3dtSqr);
        }
    }

    time_left -= dt;
}

// Kill particles with positions on wrong side of the specified domain
void PASink::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    // Must traverse list in carefully so Remove will work
    for (ParticleList::iterator it = ibegin; it != group.end(); ) {
        Particle &m = (*it);

        // Remove if inside/outside flag matches object's flag
        if(!(position->Within(m.pos) ^ kill_inside))
            group.Remove(it);
        else
            it++;
    }
}

// Kill particles with velocities on wrong side of the specified domain
void PASinkVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    // Must traverse list carefully so Remove will work
    for (ParticleList::iterator it = ibegin; it != group.end(); ) {
        Particle &m = (*it);

        // Remove if inside/outside flag matches object's flag
        if(!(velocity->Within(m.vel) ^ kill_inside))
            group.Remove(it);
        else
            it++;
    }
}

// Sort the particles by their projection onto the Look vector
void PASort::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    // First compute projection of particle onto view vector
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        pVec ToP = m.pos - Eye;
        m.tmp0 = ToP * Look;
    }

    // sort<ParticleList::iterator>(ibegin, iend);
    std::sort<Particle *>(&*ibegin, &*iend);
}

// Randomly add particles to the system
void PASource::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    PASSERT(ibegin == group.begin() && iend == group.end(), "Can only be done on whole list");

    size_t rate = size_t(floor(particle_rate * dt));

    // Dither the fractional particle in time.
    if(pRandf() < particle_rate * dt - float(rate))
        rate++;

    // Don't emit more than it can hold.
    if(group.size() + rate > group.GetMaxParticles())
        rate = group.GetMaxParticles() - group.size();

    if(vertexB_tracks) {
        for(size_t i = 0; i < rate; i++) {
            pVec pos, up, vel, rvel, siz, col, al;

            pos = position->Generate();
            up = upVec->Generate();
            vel = velocity->Generate();
            rvel = rvelocity->Generate();
            siz = size->Generate();
            col = color->Generate();
            al = alpha->Generate();
            float ag = age + pNRandf(age_sigma);

            group.Add(pos, pos, up, vel, rvel, siz, col, al.x(), ag);
        }
    } else {
        for(size_t i = 0; i < rate; i++) {
            pVec pos, posB, up, vel, rvel, siz, col, al;

            pos = position->Generate();
            posB = positionB->Generate();
            up = upVec->Generate();
            vel = velocity->Generate();
            rvel = rvelocity->Generate();
            siz = size->Generate();
            col = color->Generate();
            al = alpha->Generate();
            float ag = age + pNRandf(age_sigma);

            group.Add(pos, posB, up, vel, rvel, siz, col, al.x(), ag);
        }
    }
}

void PASpeedLimit::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float min_sqr = fsqr(min_speed);
    float max_sqr = fsqr(max_speed);

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        float sSqr = m.vel.length2();
        if(sSqr<min_sqr && sSqr) {
            float s = sqrtf(sSqr);
            m.vel *= (min_speed/s);
        } else if(sSqr>max_sqr) {
            float s = sqrtf(sSqr);
            m.vel *= (max_speed/s);
        }
    }
}

// Change color of all particles toward the specified color
void PATargetColor::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float scaleFac = scale * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        m.color += (color - m.color) * scaleFac;
        m.alpha += (alpha - m.alpha) * scaleFac;
    }
}

// Change sizes of all particles toward the specified size
void PATargetSize::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    pVec scaleFac = scale * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        pVec dif = size - m.size;
        m.size += CompMult(dif, scaleFac);
    }
}

// Change velocity of all particles toward the specified velocity
void PATargetVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float scaleFac = scale * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        m.vel += (velocity - m.vel) * scaleFac;
    }
}

// Change velocity of all particles toward the specified velocity
void PATargetRotVelocity::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float scaleFac = scale * dt;

    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);
        m.rvel += (velocity - m.rvel) * scaleFac;
    }
}

// magnitude is how much acceleration to apply at the top of the vortex (non-tip end)
// tightnessExponent is like a phong exponent that gives a curve to the vortex silhouette
// axis is the vector along the center of the vortex starting at p
// p is the tip of the vortex
void PAVortex::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
    float magdt = magnitude * dt;
    float max_radiusSqr = fsqr(max_radius);
    float axisLength = axis.length();
    float axisLengthInv = 1.0f / axisLength;
    pVec axisN = axis;
    axisN.normalize();

    // This one just rotates a particle around the axis. Amount is based on radius, magnitude, and mass.
    for (ParticleList::iterator it = ibegin; it != iend; it++) {
        Particle &m = (*it);

        // Figure direction to particle from base of line.
        pVec tipToPar = m.pos - tip;

        // Projection of particle onto line
        float axisScale = tipToPar * axisN;
        pVec parOnAxis = axisN * axisScale;

        // Direction from particle to nearest point on line.
        pVec parToAxis = parOnAxis - tipToPar;

        // Distance to axis
        float rSqr = parToAxis.length2();
        float alongAxis = axisScale * axisLengthInv;

        // This is how much to scale the vortex's force by as a function of how far up the axis the particle is.
        float alongAxisPow = powf(alongAxis, tightnessExponent);
        float silhouetteSqr = fsqr(alongAxisPow * max_radius);

        if(rSqr >= max_radiusSqr || axisScale < 0.0f || alongAxis > 1.0f) {
            // m.color = pVec(0,0,1);
            continue;
        }
        pVec AccelUp = axisN * 0.011f * dt; // XXX Make this a param.
        m.vel += AccelUp;

        if(rSqr >= silhouetteSqr) {
            // m.color = pVec(1,0,0);
            continue;
        }

        // m.color = pVec(0,1,0);
        AccelUp = axisN * -0.001f * dt; // XXX Make this a param.
        m.vel += AccelUp;

        // Apply tightness
        float r = sqrtf(rSqr);

        // Accelerate toward axis. Force drops as 1/r^2, normalize by 1/r.
        // Soften by epsilon to avoid tight encounters to infinity
        pVec AccelIn = parToAxis * ((1.0f - alongAxisPow) * magdt / (m.mass * (r * (rSqr + epsilon))));
        m.vel += AccelIn;

        // Accelerate around axis by constructing orthogonal vector frame of axis, parToAxis, and RotAccel.
        pVec RotAccel = Cross(axisN, parToAxis);
        float RA = RotAccel.length();
        float scale = rotSpeed / RA;
        RotAccel *= scale;

        pVec dst = RotAccel - parToAxis;
        float DA = dst.length();
        dst *= (r / DA);
        pVec travel = dst + parToAxis;

        pVec AccelAround = travel * (dt / m.mass);
        m.vel += AccelAround;
    }
}

// This is an experimental Action.
// It's mostly for the purpose of seeing how big the speedup can be
// if we apply all actions to a particle at once,
// rather than doing an action to all particles at once.
void PAFountain::Execute(ParticleGroup &group, ParticleList::iterator ibegin, ParticleList::iterator iend)
{
#if 0
    assert(ibegin == group.begin() && iend == group.end()); // Can only be done on whole list.

    //pGravity(0.0, 0.0, -0.01);
    //pSinkVelocity(true, PDSphere, 0, 0, 0, 0.01);
    //pBounce(-0.05, 0.35, 0, PDDisc, 0, 0, 0,  0, 0, 1,  5);
    //pSink(false, PDPlane, 0,0,-3, 0,0,1);
    //pMove();

    // For pGravity
    PAGravity *PGr = (PAGravity *)(AL+0);
    pVec ddir(PGr->direction * dt);

    PASinkVelocity *PSV = (PASinkVelocity *)(AL+1);

    // For bounce disc
    PABounce *PB = (PABounce *)(AL+2);
    float r1Sqr = fsqr(PB->position.radius1);
    float r2Sqr = fsqr(PB->position.radius2);

    // For sink
    PASink*PS = (PASink*)(AL+3);

    // Must traverse list carefully so Remove will work
    for (ParticleList::iterator it = ibegin; it != group.end(); ) {
        Particle &m = (*it);

        //pGravity(0.0, 0.0, -0.01);
        // Step velocity with acceleration
        m.vel += ddir;

        //pSinkVelocity(true, PDSphere, 0, 0, 0, 0.01);
        // Remove if inside/outside flag matches object's flag
        if(!(PSV->velocity.Within(m.vel) ^ PSV->kill_inside)) {
            group.Remove(it);
            continue;
        }

        //pBounce(-0.05, 0.35, 0, PDDisc, 0, 0, 0,  0, 0, 1,  5);

        // See if particle's current and next positions cross plane.
        // If not, couldn't bounce, so keep going.
        pVec pnext(m.pos + m.vel * dt);

        // nrm stores the plane normal (the a,b,c of the plane eqn).
        // Old and new distances: dist(p,plane) = n * p + d
        // radius1 stores -n*p, which is d. radius1Sqr stores d.
        float distold = m.pos * PB->position.nrm + PB->position.radius1Sqr;
        float distnew = pnext * PB->position.nrm + PB->position.radius1Sqr;

        if(!pSameSign(distold, distnew)) {
            // Find position at the crossing point by parameterizing
            // p(t) = pos + vel * t
            // Solve dist(p(t),plane) = 0 e.g.
            // n * p(t) + D = 0 ->
            // n * p + t (n * v) + D = 0 ->
            // t = -(n * p + D) / (n * v)
            // Could factor n*v into distnew = distold + n*v and save a bit.
            // Safe since n*v != 0 assured by quick rejection test.
            // This calc is indep. of dt because we have established that it
            // will hit before dt. We just want to know when.
            float nv = PB->position.nrm * m.vel;
            float t = -distold / nv;

            // Actual intersection point p(t) = pos + vel t
            pVec phit(m.pos + m.vel * t);

            // Offset from origin in plane, phit - origin
            pVec offset(phit - PB->position.p1);

            float rad = offset.length2();

            if(!(rad > r1Sqr || rad < r2Sqr)) {
                // A hit! A most palpable hit!

                // Compute tangential and normal components of velocity
                pVec vn(PB->position.nrm * nv); // Normal Vn = (V.N)N
                pVec vt(m.vel - vn); // Tangent Vt = V - Vn

                // Compute new velocity heading out:
                // Don't apply friction if tangential velocity < cutoff
                if(vt.length2() <= PB->cutoffSqr)
                    m.vel = vt - vn * PB->resilience;
                else
                    m.vel = vt * PB->oneMinusFriction - vn * PB->resilience;
            }
        }

        //pSink(false, PDPlane, 0,0,-3, 0,0,1);
        // Remove if inside/outside flag matches object's flag
        if(!(PS->position.Within(m.pos) ^ PS->kill_inside)) {
            group.Remove(it);
            continue;
        } else
            it++;

        //pMove();
        m.age += dt;
        m.pos += m.vel * dt;
    }
#endif
}
