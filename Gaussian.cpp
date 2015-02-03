#include "Gaussian.h"
#include "Transform.h"


// Implementation of recursive Gaussian algorithm from "Ian T. Young, Lucas J. van Vliet - Recursive implementation of the Gaussian filter"
Plane &Gaussian2D(Plane &dst, const Plane &src, const double sigma)
{
    if (sigma <= 0)
    {
        dst = src;
        return dst;
    }

    Plane_FL data(src);
    RecursiveGaussian GFilter(sigma);

    GFilter.Filter(data);
    dst.From(data);
    
    return dst;
}


// Member functions of class RecursiveGaussian
void RecursiveGaussian::GetPara(double sigma)
{
    const double q = sigma < 2.5 ? 3.97156 - 4.14554*sqrt(1 - 0.26891*sigma) : 0.98711*sigma - 0.96330;

    const double b0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
    const double b1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
    const double b2 = -(1.4281*q*q + 1.26661*q*q*q);
    const double b3 = 0.422205*q*q*q;

    B = static_cast<FLType>(1 - (b1 + b2 + b3) / b0);
    B1 = static_cast<FLType>(b1 / b0);
    B2 = static_cast<FLType>(b2 / b0);
    B3 = static_cast<FLType>(b3 / b0);
}


void RecursiveGaussian::FilterV(Plane_FL &dst, const Plane_FL &src)
{
    PCType i0, i1, i2, i3, j, lower, upper;
    PCType height = src.Height();
    PCType width = src.Width();
    PCType stride = src.Width();
    FLType P0, P1, P2, P3;

    if (dst.Data() != src.Data())
    {
        memcpy(dst.Data(), src.Data(), sizeof(FLType) * width);
    }

    for (j = 0; j < height; ++j)
    {
        lower = j * stride;
        upper = lower + width;

        i0 = lower;
        i1 = j < 1 ? i0 : i0 - stride;
        i2 = j < 2 ? i1 : i1 - stride;
        i3 = j < 3 ? i2 : i2 - stride;

        for (; i0 < upper; ++i0, ++i1, ++i2, ++i3)
        {
            P3 = dst[i3];
            P2 = dst[i2];
            P1 = dst[i1];
            P0 = src[i0];
            dst[i0] = B * P0 + B1 * P1 + B2 * P2 + B3 * P3;
        }
    }

    for (j = height - 1; j >= 0; --j)
    {
        lower = j * stride;
        upper = lower + width;

        i0 = lower;
        i1 = j >= height - 1 ? i0 : i0 + stride;
        i2 = j >= height - 2 ? i1 : i1 + stride;
        i3 = j >= height - 3 ? i2 : i2 + stride;

        for (; i0 < upper; ++i0, ++i1, ++i2, ++i3)
        {
            P3 = dst[i3];
            P2 = dst[i2];
            P1 = dst[i1];
            P0 = dst[i0];
            dst[i0] = B * P0 + B1 * P1 + B2 * P2 + B3 * P3;
        }
    }
}

void RecursiveGaussian::FilterV(Plane_FL &data)
{
    PCType i0, i1, i2, i3, j, lower, upper;
    PCType height = data.Height();
    PCType width = data.Width();
    PCType stride = data.Width();
    FLType P0, P1, P2, P3;

    for (j = 0; j < height; ++j)
    {
        lower = j * stride;
        upper = lower + width;

        i0 = lower;
        i1 = j < 1 ? i0 : i0 - stride;
        i2 = j < 2 ? i1 : i1 - stride;
        i3 = j < 3 ? i2 : i2 - stride;

        for (; i0 < upper; ++i0, ++i1, ++i2, ++i3)
        {
            P3 = data[i3];
            P2 = data[i2];
            P1 = data[i1];
            P0 = data[i0];
            data[i0] = B * P0 + B1 * P1 + B2 * P2 + B3 * P3;
        }
    }

    for (j = height - 1; j >= 0; --j)
    {
        lower = j * stride;
        upper = lower + width;

        i0 = lower;
        i1 = j >= height - 1 ? i0 : i0 + stride;
        i2 = j >= height - 2 ? i1 : i1 + stride;
        i3 = j >= height - 3 ? i2 : i2 + stride;

        for (; i0 < upper; ++i0, ++i1, ++i2, ++i3)
        {
            P3 = data[i3];
            P2 = data[i2];
            P1 = data[i1];
            P0 = data[i0];
            data[i0] = B * P0 + B1 * P1 + B2 * P2 + B3 * P3;
        }
    }
}


void RecursiveGaussian::FilterH(Plane_FL &dst, const Plane_FL &src)
{
    PCType i, j, lower, upper;
    PCType height = src.Height();
    PCType width = src.Width();
    PCType stride = src.Width();
    FLType P0, P1, P2, P3;
    
    for (j = 0; j < height; ++j)
    {
        lower = j * stride;
        upper = lower + width;

        i = lower;
        dst[i] = P3 = P2 = P1 = src[i];

        for (++i; i < upper; ++i)
        {
            P0 = B * src[i] + B1 * P1 + B2 * P2 + B3 * P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            dst[i] = P0;
        }
        
        --i;
        P3 = P2 = P1 = dst[i];

        for (--i; i >= lower; --i)
        {
            P0 = B * dst[i] + B1 * P1 + B2 * P2 + B3 * P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            dst[i] = P0;
        }
    }
}

void RecursiveGaussian::FilterH(Plane_FL &data)
{
    PCType i, j, lower, upper;
    PCType height = data.Height();
    PCType width = data.Width();
    PCType stride = data.Width();
    FLType P0, P1, P2, P3;

    for (j = 0; j < height; ++j)
    {
        lower = j * stride;
        upper = lower + width;

        i = lower;
        P3 = P2 = P1 = data[i];

        for (++i; i < upper; ++i)
        {
            P0 = B * data[i] + B1 * P1 + B2 * P2 + B3 * P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            data[i] = P0;
        }

        --i;
        P3 = P2 = P1 = data[i];

        for (--i; i >= lower; --i)
        {
            P0 = B * data[i] + B1 * P1 + B2 * P2 + B3 * P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            data[i] = P0;
        }
    }
}
