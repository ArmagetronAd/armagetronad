#ifndef PDOMAIN_H_INCLUDED
#define PDOMAIN_H_INCLUDED

#include "pVec.h"

class pDomain
{
public:
    virtual bool Within(const pVec &) const = 0;
    virtual pVec Generate() const = 0;

    virtual pDomain *copy() const = 0; // Returns a pointer to a heap-allocated copy of the derived class

    virtual ~pDomain() {}
};

// Single point
class PDPoint : public pDomain
{
public:
    pVec p;

public:
    PDPoint(const pVec &p0)
    {
        p = p0;
    }

    ~PDPoint()
    {
    }

    bool Within(const pVec &pos) const
    {
        return false;
    }

    pVec Generate() const
    {
        return p;
    }

    pDomain *copy() const
    {
        PDPoint *P = new PDPoint(*this);
        return P;
    }
};

// Line segment
class PDLine : public pDomain
{
public:
    pVec p0, p1;

public:
    PDLine(const pVec &e0, const pVec &e1)
    {
        p0 = e0;
        p1 = e1 - e0;
    }

    ~PDLine()
    {
    }

    bool Within(const pVec &pos) const
    {
        return false;
    }

    pVec Generate() const
    {
        return p0 + p1 * pRandf();
    }

    pDomain *copy() const
    {
        PDLine *P = new PDLine(*this);
        return P;
    }
};

// Triangle
class PDTriangle : public pDomain
{
public:
    pVec p, u, v, uNrm, vNrm, nrm;
    float uLen, vLen, D;

public:
    PDTriangle(const pVec &p0, const pVec &p1, const pVec &p2)
    {
        p = p0;
        u = p1 - p0;
        v = p2 - p0;

        // The rest of this is needed for Avoid and Bounce.
        uLen = u.length();
        uNrm = u / uLen;
        vLen = v.length();
        vNrm = v / vLen;

        nrm = Cross(uNrm, vNrm); // This is the non-unit normal.
        nrm.normalize(); // Must normalize it.

        D = -(p * nrm);
    }

    ~PDTriangle()
    {
    }

    bool Within(const pVec &pos) const
    {
        return false;
    }

    pVec Generate() const
    {
        float r1 = pRandf();
        float r2 = pRandf();
        pVec pos;
        if(r1 + r2 < 1.0f)
            pos = p + u * r1 + v * r2;
        else
            pos = p + u * (1.0f-r1) + v * (1.0f-r2);

        return pos;
    }

    pDomain *copy() const
    {
        PDTriangle *P = new PDTriangle(*this);
        return P;
    }
};

// Rhombus-shaped planar region
class PDRectangle : public pDomain
{
public:
    pVec p, u, v, uNrm, vNrm, nrm;
    float uLen, vLen, D;

public:
    PDRectangle(const pVec &p0, const pVec &u0, const pVec &v0)
    {
        p = p0;
        u = u0;
        v = v0;

        // The rest of this is needed for Avoid and Bounce.
        uLen = u.length();
        uNrm = u / uLen;
        vLen = v.length();
        vNrm = v / vLen;

        nrm = Cross(uNrm, vNrm); // This is the non-unit normal.
        nrm.normalize(); // Must normalize it.

        D = -(p * nrm);
    }

    ~PDRectangle()
    {
        // std::cerr << "del " << typeid(*this).name() << this << std::endl;
    }

    bool Within(const pVec &pos) const
    {
        return false;
    }

    pVec Generate() const
    {
        pVec pos = p + u * pRandf() + v * pRandf();
        return pos;
    }

    pDomain *copy() const
    {
        PDRectangle *P = new PDRectangle(*this);
        return P;
    }
};

// Arbitrarily-oriented plane
class PDPlane : public pDomain
{
public:
    pVec p, nrm;
    float D;

public:
    PDPlane(const pVec &p0, const pVec &nrm0)
    {
        p = p0;
        nrm = nrm0;
        nrm.normalize(); // Must normalize it.
        D = -(p * nrm);
    }

    ~PDPlane()
    {
    }

    // Distance from plane = n * p + d
    // Inside is the positive half-space.
    bool Within(const pVec &pos) const
    {
        return nrm * pos >= -D;
    }

    // How do I sensibly make a point on an infinite plane?
    pVec Generate() const
    {
        return p;
    }

    pDomain *copy() const
    {
        PDPlane *P = new PDPlane(*this);
        return P;
    }
};

// Axis-aligned box
class PDBox : public pDomain
{
public:
    // p0 is the min corner. p1 is the max corner.
    pVec p0, p1, dif;

public:
    PDBox(const pVec &e0, const pVec &e1)
    {
        p0 = e0;
        p1 = e1;
        if(e1.x() < e0.x()) { p0.x() = e1.x(); p1.x() = e1.x(); }
        if(e1.y() < e0.y()) { p0.y() = e1.y(); p1.y() = e1.y(); }
        if(e1.z() < e0.z()) { p0.z() = e1.z(); p1.z() = e1.z(); }

        dif = p1 - p0;
    }

    ~PDBox()
    {
    }

    bool Within(const pVec &pos) const
    {
        return !((pos.x() < p0.x()) || (pos.x() > p1.x()) ||
                 (pos.y() < p0.y()) || (pos.y() > p1.y()) ||
                 (pos.z() < p0.z()) || (pos.z() > p1.z()));
    }

    pVec Generate() const
    {
        // Scale and translate [0,1] random to fit box
        return p0 + CompMult(pRandVec(), dif);
    }

    pDomain *copy() const
    {
        PDBox *P = new PDBox(*this);
        return P;
    }
};

// Cylinder
class PDCylinder : public pDomain
{
public:
    pVec apex, axis, u, v; // Apex is one end. Axis is vector from one end to the other.
    float len, radOut, radIn, radOutSqr, radInSqr, radDif, axisLenInvSqr;
    bool ThinShell;

public:
    PDCylinder(const pVec &e0, const pVec &e1, const float radOut0, const float radIn0 = 0.0f)
    {
        apex = e0;
        axis = e1 - e0;

        if(radOut0 < radIn0) {
            radOut = radIn0; radIn = radOut0;
        } else {
            radOut = radOut0; radIn = radIn0;
        }
        radOutSqr = fsqr(radOut);
        radInSqr = fsqr(radIn);

        ThinShell = (radIn == radOut);
        radDif = radOut - radIn;

        // Given an arbitrary nonzero vector n, make two orthonormal
        // vectors u and v forming a frame [u,v,n.normalize()].
        pVec n = axis;
        float axisLenSqr = axis.length2();
        axisLenInvSqr = axisLenSqr ? (1.0f / axisLenSqr) : 0.0f;
        n *= sqrtf(axisLenInvSqr);

        // Find a vector orthogonal to n.
        pVec basis(1.0f, 0.0f, 0.0f);
        if (fabsf(basis * n) > 0.999f)
            basis = pVec(0.0f, 1.0f, 0.0f);

        // Project away N component, normalize and cross to get
        // second orthonormal vector.
        u = basis - n * (basis * n);
        u.normalize();
        v = Cross(n, u);
    }

    ~PDCylinder()
    {
    }

    bool Within(const pVec &pos) const
    {
        // This is painful and slow. Might be better to do quick accept/reject tests.
        // Axis is vector from base to tip of the cylinder.
        // x is vector from base to pos.
        //         x . axis
        // dist = ---------- = projected distance of x along the axis
        //        axis. axis   ranging from 0 (base) to 1 (tip)
        //
        // rad = x - dist * axis = projected vector of x along the base

        pVec x = pos - apex;

        // Check axial distance
        float dist = (axis * x) * axisLenInvSqr;
        if(dist < 0.0f || dist > 1.0f)
            return false;

        // Check radial distance
        pVec xrad = x - axis * dist; // Radial component of x
        float rSqr = xrad.length2();

        return (rSqr <= radInSqr && rSqr >= radOutSqr);
    }

    pVec Generate() const
    {
        float dist = pRandf(); // Distance between base and tip
        float theta = pRandf() * 2.0f * float(M_PI); // Angle around axis
        // Distance from axis
        float r = radIn + pRandf() * radDif;

        // Another way to do this is to choose a random point in a square and keep it if it's in the circle.
        float x = r * cosf(theta);
        float y = r * sinf(theta);

        pVec pos = apex + axis * dist + u * x + v * y;
        return pos;
    }

    pDomain *copy() const
    {
        PDCylinder *P = new PDCylinder(*this);
        return P;
    }
};

// Cone
class PDCone : public pDomain
{
public:
    pVec apex, axis, u, v; // Apex is one end. Axis is vector from one end to the other.
    float len, radOut, radIn, radOutSqr, radInSqr, radDif, axisLenInvSqr;
    bool ThinShell;

public:
    PDCone(const pVec &e0, const pVec &e1, const float radOut0, const float radIn0 = 0.0f)
    {
        apex = e0;
        axis = e1 - e0;

        if(radOut0 < radIn0) {
            radOut = radIn0; radIn = radOut0;
        } else {
            radOut = radOut0; radIn = radIn0;
        }
        radOutSqr = fsqr(radOut);
        radInSqr = fsqr(radIn);

        ThinShell = (radIn == radOut);
        radDif = radOut - radIn;

        // Given an arbitrary nonzero vector n, make two orthonormal
        // vectors u and v forming a frame [u,v,n.normalize()].
        pVec n = axis;
        float axisLenSqr = axis.length2();
        axisLenInvSqr = axisLenSqr ? 1.0f / axisLenSqr : 0.0f;
        n *= sqrtf(axisLenInvSqr);

        // Find a vector orthogonal to n.
        pVec basis(1.0f, 0.0f, 0.0f);
        if (fabsf(basis * n) > 0.999f)
            basis = pVec(0.0f, 1.0f, 0.0f);

        // Project away N component, normalize and cross to get
        // second orthonormal vector.
        u = basis - n * (basis * n);
        u.normalize();
        v = Cross(n, u);
    }

    ~PDCone()
    {
    }

    bool Within(const pVec &pos) const
    {
        // This is painful and slow. Might be better to do quick
        // accept/reject tests.
        // Let axis = vector from base to tip of the cylinder
        // x = vector from base to test point
        //         x . axis
        // dist = ---------- = projected distance of x along the axis
        //        axis. axis   ranging from 0 (base) to 1 (tip)
        //
        // rad = x - dist * axis = projected vector of x along the base

        pVec x = pos - apex;

        // Check axial distance
        // axisLenInvSqr stores 1 / (axis.axis)
        float dist = (axis * x) * axisLenInvSqr;
        if(dist < 0.0f || dist > 1.0f)
            return false;

        // Check radial distance; scale radius along axis for cones
        pVec xrad = x - axis * dist; // Radial component of x
        float rSqr = xrad.length2();

        return (rSqr <= fsqr(dist * radIn) && rSqr >= fsqr(dist * radOut));
    }

    pVec Generate() const
    {
        float dist = pRandf(); // Distance between base and tip
        float theta = pRandf() * 2.0f * float(M_PI); // Angle around axis
        // Distance from axis
        float r = radIn + pRandf() * radDif;

        // Another way to do this is to choose a random point in a square and keep it if it's in the circle.
        float x = r * cosf(theta);
        float y = r * sinf(theta);

        // Scale radius along axis for cones
        x *= dist;
        y *= dist;

        pVec pos = apex + axis * dist + u * x + v * y;
        return pos;
    }

    pDomain *copy() const
    {
        PDCone *P = new PDCone(*this);
        return P;
    }
};

// Sphere
class PDSphere : public pDomain
{
public:
    pVec ctr;
    float radOut, radIn, radOutSqr, radInSqr, radDif;
    bool ThinShell;

public:
    PDSphere(const pVec &ctr0, const float radOut0, const float radIn0 = 0.0f)
    {
        ctr = ctr0;
        if(radOut0 < radIn0) {
            radOut = radIn0; radIn = radOut0;
        } else {
            radOut = radOut0; radIn = radIn0;
        }
        if(radIn < 0.0f) radIn = 0.0f;

        radOutSqr = fsqr(radOut);
        radInSqr = fsqr(radIn);

        ThinShell = (radIn == radOut);
        radDif = radOut - radIn;
    }

    ~PDSphere()
    {
    }

    bool Within(const pVec &pos) const
    {
        pVec rvec(pos - ctr);
        float rSqr = rvec.length2();
        return rSqr <= radOutSqr && rSqr >= radInSqr;
    }

    pVec Generate() const
    {
        pVec pos;

        do {
            pos = pRandVec() - vHalf; // Point on [-0.5,0.5] box
        } while (pos.length2() > fsqr(0.5)); // Make sure it's also on r=0.5 sphere.
        pos.normalize(); // Now it's on r=1 spherical shell

        // Scale unit sphere pos by [0..r] and translate
        if(ThinShell)
            pos = ctr + pos * radOut;
        else
            pos = ctr + pos * (radIn + pRandf() * radDif);

        return pos;
    }

    pDomain *copy() const
    {
        PDSphere *P = new PDSphere(*this);
        return P;
    }
};

// Gaussian blob
class PDBlob : public pDomain
{
public:
    pVec ctr;
    float stdev, Scale1, Scale2;

public:
    PDBlob(const pVec &ctr0, const float stdev0)
    {
        ctr = ctr0;
        stdev = stdev0;
        float oneOverSigma = 1.0f/(stdev+0.000000000001f);
        Scale1 = -0.5f*fsqr(oneOverSigma);
        Scale2 = P_ONEOVERSQRT2PI * oneOverSigma;
    }

    ~PDBlob()
    {
    }

    bool Within(const pVec &pos) const
    {
        pVec x = pos - ctr;
        // return exp(-0.5 * xSq * Sqr(oneOverSigma)) * P_ONEOVERSQRT2PI * oneOverSigma;
        float Gx = expf(x.length2() * Scale1) * Scale2;
        return (pRandf() < Gx);
    }

    pVec Generate() const
    {
        return ctr + pNRandVec(stdev);
    }

    pDomain *copy() const
    {
        PDBlob *P = new PDBlob(*this);
        return P;
    }
};

// Arbitrarily-oriented disc
class PDDisc : public pDomain
{
public:
    pVec p, nrm, u, v;
    float radIn, radOut, radInSqr, radOutSqr, dif, D;

public:
    PDDisc(const pVec &ctr0, const pVec nrm0, const float radOut0, const float radIn0 = 0.0f)
    {
        p = ctr0;
        nrm = nrm0;
        nrm.normalize();

        if(radOut0 > radIn0) {
            radOut = radOut0; radIn = radIn0;
        } else {
            radOut = radIn0; radIn = radOut0;
        }
        dif = radOut - radIn;
        radInSqr = fsqr(radIn);
        radOutSqr = fsqr(radOut);

        // Find a vector orthogonal to n.
        pVec basis(1.0f, 0.0f, 0.0f);
        if (fabsf(basis * nrm) > 0.999f)
            basis = pVec(0.0f, 1.0f, 0.0f);

        // Project away N component, normalize and cross to get
        // second orthonormal vector.
        u = basis - nrm * (basis * nrm);
        u.normalize();
        v = Cross(nrm, u);
        D = -(p * nrm);
    }

    ~PDDisc()
    {
    }

    bool Within(const pVec &pos) const
    {
        return false;
    }

    pVec Generate() const
    {
        // Might be faster to generate a point in a square and reject if outside the circle
        float theta = pRandf() * 2.0f * float(M_PI); // Angle around normal
        // Distance from center
        float r = radIn + pRandf() * dif;

        float x = r * cosf(theta); // Weighting of each frame vector
        float y = r * sinf(theta);

        pVec pos = p + u * x + v * y;
        return pos;
    }

    pDomain *copy() const
    {
        PDDisc *P = new PDDisc(*this);
        return P;
    }
};

#endif // PDOMAIN_H_INCLUDED
