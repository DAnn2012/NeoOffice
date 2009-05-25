/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 * 
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include <tools/solar.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>

#include "interpre.hxx"
#include "global.hxx"
#include "compiler.hxx"
#include "cell.hxx"
#include "document.hxx"
#include "dociter.hxx"
#include "scmatrix.hxx"
#include "globstr.hrc"

using ::std::vector;

// STATIC DATA -----------------------------------------------------------

#define SCdEpsilon                1.0E-7
#define SC_MAX_ITERATION_COUNT    20
#define MAX_ANZ_DOUBLE_FOR_SORT 100000
// PI jetzt als F_PI aus solar.h
//#define   PI            3.1415926535897932

const double ScInterpreter::fMaxGammaArgument = 171.624376956302;  // found experimental

//-----------------------------------------------------------------------------

class ScDistFunc
{
public:
    virtual double GetValue(double x) const = 0;
};

//  iteration for inverse distributions

//template< class T > double lcl_IterateInverse( const T& rFunction, double x0, double x1, bool& rConvError )

/** u*w<0.0 fails for values near zero */
inline bool lcl_HasChangeOfSign( double u, double w )
{
    return (u < 0.0 && w > 0.0) || (u > 0.0 && w < 0.0);
}

double lcl_IterateInverse( const ScDistFunc& rFunction, double fAx, double fBx, bool& rConvError )
{
    rConvError = false;
    const double fYEps = 1.0E-307;
    const double fXEps = ::std::numeric_limits<double>::epsilon();

    DBG_ASSERT(fAx<fBx, "IterateInverse: wrong interval");

    //  find enclosing interval

    double fAy = rFunction.GetValue(fAx);
    double fBy = rFunction.GetValue(fBx);
    double fTemp;
    unsigned short nCount;
    for (nCount = 0; nCount < 1000 && !lcl_HasChangeOfSign(fAy,fBy); nCount++)
    {
        if (fabs(fAy) <= fabs(fBy))
        {
            fTemp = fAx;
            fAx += 2.0 * (fAx - fBx);
            if (fAx < 0.0)
                fAx = 0.0;
            fBx = fTemp;
            fBy = fAy;
            fAy = rFunction.GetValue(fAx);
        }
        else
        {
            fTemp = fBx;
            fBx += 2.0 * (fBx - fAx);
            fAx = fTemp;
            fAy = fBy;
            fBy = rFunction.GetValue(fBx);
        }
    }

    if (fAy == 0.0)
        return fAx;
    if (fBy == 0.0)
        return fBx;
    if (!lcl_HasChangeOfSign( fAy, fBy))
    {
        rConvError = true;
        return 0.0;
    }
    // inverse quadric interpolation with additional brackets
    // set three points
    double fPx = fAx;
    double fPy = fAy;
    double fQx = fBx;
    double fQy = fBy;
    double fRx = fAx;
    double fRy = fAy;
    double fSx = 0.5 * (fAx + fBx); // potential next point
    bool bHasToInterpolate = true;
    nCount = 0;
    while ( nCount < 500 && fabs(fRy) > fYEps &&
            (fBx-fAx) > ::std::max( fabs(fAx), fabs(fBx)) * fXEps )
    {
        if (bHasToInterpolate)
        {
            if (fPy!=fQy && fQy!=fRy && fRy!=fPy)
            {
                fSx = fPx * fRy * fQy / (fRy-fPy) / (fQy-fPy)
                    + fRx * fQy * fPy / (fQy-fRy) / (fPy-fRy)
                    + fQx * fPy * fRy / (fPy-fQy) / (fRy-fQy);
                bHasToInterpolate = (fAx < fSx) && (fSx < fBx); // inside the brackets?
            }
            else
                bHasToInterpolate = false;
        }
        if(!bHasToInterpolate)
        {
            fSx = 0.5 * (fAx + fBx);
            // reset points
            fPx = fAx; fPy = fAy;
            fQx = fBx; fQy = fBy;
            bHasToInterpolate = true;
        }
        // shift points for next interpolation
        fPx = fQx; fQx = fRx; fRx = fSx;
        fPy = fQy; fQy = fRy; fRy = rFunction.GetValue(fSx);
        // update brackets
        if (lcl_HasChangeOfSign( fAy, fRy))
        {
            fBx = fRx; fBy = fRy;
        }
        else
        {
            fAx = fRx; fAy = fRy;
        }
        // if last interration brought to small advance, then do bisection next
        // time, for safety
        bHasToInterpolate = bHasToInterpolate && (fabs(fRy) * 2.0 <= fabs(fQy));
        ++nCount;
    }
    return fRx;
}

//-----------------------------------------------------------------------------
// Allgemeine Funktionen
//-----------------------------------------------------------------------------

void ScInterpreter::ScNoName()
{
    PushError(errNoName);
}

void ScInterpreter::ScBadName()
{
    short nParamCount = GetByte();
    while (nParamCount-- > 0)
    {
        PopError();
    }
    PushError( errNoName);
}

double ScInterpreter::phi(double x)
{
    return  0.39894228040143268 * exp(-(x * x) / 2.0);
}

double ScInterpreter::taylor(double* pPolynom, USHORT nMax, double x)
{
    double nVal = pPolynom[nMax];
    for (short i = nMax-1; i >= 0; i--)
    {
        nVal = pPolynom[i] + (nVal * x);
    }
    return nVal;
}

double ScInterpreter::gauss(double x)
{
    double t0[] =
    { 0.39894228040143268, -0.06649038006690545,  0.00997355701003582,
     -0.00118732821548045,  0.00011543468761616, -0.00000944465625950,
      0.00000066596935163, -0.00000004122667415,  0.00000000227352982,
      0.00000000011301172,  0.00000000000511243, -0.00000000000021218 };
    double t2[] =
    { 0.47724986805182079,  0.05399096651318805, -0.05399096651318805,
      0.02699548325659403, -0.00449924720943234, -0.00224962360471617,
      0.00134977416282970, -0.00011783742691370, -0.00011515930357476,
      0.00003704737285544,  0.00000282690796889, -0.00000354513195524,
      0.00000037669563126,  0.00000019202407921, -0.00000005226908590,
     -0.00000000491799345,  0.00000000366377919, -0.00000000015981997,
     -0.00000000017381238,  0.00000000002624031,  0.00000000000560919,
     -0.00000000000172127, -0.00000000000008634,  0.00000000000007894 };
    double t4[] =
    { 0.49996832875816688,  0.00013383022576489, -0.00026766045152977,
      0.00033457556441221, -0.00028996548915725,  0.00018178605666397,
     -0.00008252863922168,  0.00002551802519049, -0.00000391665839292,
     -0.00000074018205222,  0.00000064422023359, -0.00000017370155340,
      0.00000000909595465,  0.00000000944943118, -0.00000000329957075,
      0.00000000029492075,  0.00000000011874477, -0.00000000004420396,
      0.00000000000361422,  0.00000000000143638, -0.00000000000045848 };
    double asympt[] = { -1.0, 1.0, -3.0, 15.0, -105.0 };

    double xAbs = fabs(x);
    USHORT xShort = (USHORT)::rtl::math::approxFloor(xAbs);
    double nVal = 0.0;
    if (xShort == 0)
        nVal = taylor(t0, 11, (xAbs * xAbs)) * xAbs;
    else if ((xShort >= 1) && (xShort <= 2))
        nVal = taylor(t2, 23, (xAbs - 2.0));
    else if ((xShort >= 3) && (xShort <= 4))
        nVal = taylor(t4, 20, (xAbs - 4.0));
    else
        nVal = 0.5 + phi(xAbs) * taylor(asympt, 4, 1.0 / (xAbs * xAbs)) / xAbs;
    if (x < 0.0)
        return -nVal;
    else
        return nVal;
}

//
//  #i26836# new gaussinv implementation by Martin Eitzenberger <m.eitzenberger@unix.net>
//

double ScInterpreter::gaussinv(double x)
{
    double q,t,z;

    q=x-0.5;

    if(fabs(q)<=.425)
    {
        t=0.180625-q*q;

        z=
        q*
        (
            (
                (
                    (
                        (
                            (
                                (
                                    t*2509.0809287301226727+33430.575583588128105
                                )
                                *t+67265.770927008700853
                            )
                            *t+45921.953931549871457
                        )
                        *t+13731.693765509461125
                    )
                    *t+1971.5909503065514427
                )
                *t+133.14166789178437745
            )
            *t+3.387132872796366608
        )
        /
        (
            (
                (
                    (
                        (
                            (
                                (
                                    t*5226.495278852854561+28729.085735721942674
                                )
                                *t+39307.89580009271061
                            )
                            *t+21213.794301586595867
                        )
                        *t+5394.1960214247511077
                    )
                    *t+687.1870074920579083
                )
                *t+42.313330701600911252
            )
            *t+1.0
        );

    }
    else
    {
        if(q>0) t=1-x;
        else        t=x;

        t=sqrt(-log(t));

        if(t<=5.0)
        {
            t+=-1.6;

            z=
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        t*7.7454501427834140764e-4+0.0227238449892691845833
                                    )
                                    *t+0.24178072517745061177
                                )
                                *t+1.27045825245236838258
                            )
                            *t+3.64784832476320460504
                        )
                        *t+5.7694972214606914055
                    )
                    *t+4.6303378461565452959
                )
                *t+1.42343711074968357734
            )
            /
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        t*1.05075007164441684324e-9+5.475938084995344946e-4
                                    )
                                    *t+0.0151986665636164571966
                                )
                                *t+0.14810397642748007459
                            )
                            *t+0.68976733498510000455
                        )
                        *t+1.6763848301838038494
                    )
                    *t+2.05319162663775882187
                )
                *t+1.0
            );

        }
        else
        {
            t+=-5.0;

            z=
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        t*2.01033439929228813265e-7+2.71155556874348757815e-5
                                    )
                                    *t+0.0012426609473880784386
                                )
                                *t+0.026532189526576123093
                            )
                            *t+0.29656057182850489123
                        )
                        *t+1.7848265399172913358
                    )
                    *t+5.4637849111641143699
                )
                *t+6.6579046435011037772
            )
            /
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        t*2.04426310338993978564e-15+1.4215117583164458887e-7
                                    )
                                    *t+1.8463183175100546818e-5
                                )
                                *t+7.868691311456132591e-4
                            )
                            *t+0.0148753612908506148525
                        )
                        *t+0.13692988092273580531
                    )
                    *t+0.59983220655588793769
                )
                *t+1.0
            );

        }

        if(q<0.0) z=-z;
    }

    return z;
}

double ScInterpreter::Fakultaet(double x)
{
    x = ::rtl::math::approxFloor(x);
    if (x < 0.0)
        return 0.0;
    else if (x == 0.0)
        return 1.0;
    else if (x <= 170.0)
    {
        double fTemp = x;
        while (fTemp > 2.0)
        {
            fTemp--;
            x *= fTemp;
        }
    }
    else
        SetError(errNoValue);
/*                                           // Stirlingsche Naeherung zu ungenau
    else
        x = pow(x/exp(1), x) * sqrt(x) * SQRT_2_PI * (1.0 + 1.0 / (12.0 * x));
*/
    return x;
}

double ScInterpreter::BinomKoeff(double n, double k)
{
    double nVal = 0.0;
    k = ::rtl::math::approxFloor(k);
    if (n < k)
        nVal = 0.0;
    else if (k == 0.0)
        nVal = 1.0;
    else
    {
        nVal = n/k;
        n--;
        k--;
        while (k > 0.0)
        {
            nVal *= n/k;
            k--;
            n--;
        }
/*
        double f1 = n;                      // Zaehler
        double f2 = k;                      // Nenner
        n--;
        k--;
        while (k > 0.0)
        {
            f2 *= k;
            f1 *= n;
            k--;
            n--;
        }
        nVal = f1 / f2;
*/
    }
    return nVal;
}


// The algorithm is based on lanczos13m53 in lanczos.hpp
// in math library from http://www.boost.org
/** you must ensure fZ>0
    Uses a variant of the Lanczos sum with a rational function. */
double lcl_getLanczosSum(double fZ)
{
    const double fNum[13] ={
        23531376880.41075968857200767445163675473,
        42919803642.64909876895789904700198885093,
        35711959237.35566804944018545154716670596,
        17921034426.03720969991975575445893111267,
        6039542586.35202800506429164430729792107,
        1439720407.311721673663223072794912393972,
        248874557.8620541565114603864132294232163,
        31426415.58540019438061423162831820536287,
        2876370.628935372441225409051620849613599,
        186056.2653952234950402949897160456992822,
        8071.672002365816210638002902272250613822,
        210.8242777515793458725097339207133627117,
        2.506628274631000270164908177133837338626
        };
    const double fDenom[13] = {
        0,
        39916800,
        120543840,
        150917976,
        105258076,
        45995730,
        13339535,
        2637558,
        357423,
        32670,
        1925,
        66,
        1
        };
    // Horner scheme
    double fSumNum;
    double fSumDenom;
    int nI;
    double fZInv;
    if (fZ<=1.0)
    {
        fSumNum = fNum[12];
        fSumDenom = fDenom[12];
        for (nI = 11; nI >= 0; --nI)
        {
            fSumNum *= fZ;
            fSumNum += fNum[nI];
            fSumDenom *= fZ;
            fSumDenom += fDenom[nI];
        }
    }
    else
    // Cancel down with fZ^12; Horner scheme with reverse coefficients
    {
        fZInv = 1/fZ;
        fSumNum = fNum[0];
        fSumDenom = fDenom[0];
        for (nI = 1; nI <=12; ++nI)
        {
            fSumNum *= fZInv;
            fSumNum += fNum[nI];
            fSumDenom *= fZInv;
            fSumDenom += fDenom[nI];
        }
    }
    return fSumNum/fSumDenom;
}

// The algorithm is based on tgamma in gamma.hpp
// in math library from http://www.boost.org
/** You must ensure fZ>0; fZ>171.624376956302 will overflow. */
double lcl_GetGammaHelper(double fZ)
{
    double fGamma = lcl_getLanczosSum(fZ);
    const double fg = 6.024680040776729583740234375;
    double fZgHelp = fZ + fg - 0.5;
    // avoid intermediate overflow
    double fHalfpower = pow( fZgHelp, fZ / 2 - 0.25);
    fGamma *= fHalfpower;
    fGamma /= exp(fZgHelp);
    fGamma *= fHalfpower;
    if (fZ <= 20.0 && fZ == ::rtl::math::approxFloor(fZ))
        fGamma = ::rtl::math::round(fGamma);
    return fGamma;
}

// The algorithm is based on tgamma in gamma.hpp
// in math library from http://www.boost.org
/** You must ensure fZ>0 */
double lcl_GetLogGammaHelper(double fZ)
{
    const double fg = 6.024680040776729583740234375;
    double fZgHelp = fZ + fg - 0.5;
    return log( lcl_getLanczosSum(fZ)) + (fZ-0.5) * log(fZgHelp) - fZgHelp;
}

/** You must ensure non integer arguments for fZ<1 */
double ScInterpreter::GetGamma(double fZ)
{
    const double fLogPi = log(F_PI);
    const double fLogDblMax = log( ::std::numeric_limits<double>::max());

    if (fZ > fMaxGammaArgument)
    {
        SetError(errIllegalFPOperation);
        return HUGE_VAL;
    }

    if (fZ >= 1.0)
        return lcl_GetGammaHelper(fZ);

    if (fZ >= 0.5)  // shift to x>=1 using Gamma(x)=Gamma(x+1)/x
        return lcl_GetGammaHelper(fZ+1) / fZ;

    if (fZ >= -0.5) // shift to x>=1, might overflow
    {
        double fLogTest = lcl_GetLogGammaHelper(fZ+2) - log(fZ+1) - log( fabs(fZ));
        if (fLogTest >= fLogDblMax)
        {
            SetError( errIllegalFPOperation);
            return HUGE_VAL;
        }
        return lcl_GetGammaHelper(fZ+2) / (fZ+1) / fZ;
    }
    // fZ<-0.5
    // Use Euler's reflection formula: gamma(x)= pi/ ( gamma(1-x)*sin(pi*x) )
    double fLogDivisor = lcl_GetLogGammaHelper(1-fZ) + log( fabs( ::rtl::math::sin( F_PI*fZ)));
    if (fLogDivisor - fLogPi >= fLogDblMax)     // underflow
        return 0.0;

    if (fLogDivisor<0.0)
        if (fLogPi - fLogDivisor > fLogDblMax)  // overflow
        {
            SetError(errIllegalFPOperation);
            return HUGE_VAL;
        }

    return exp( fLogPi - fLogDivisor) * ((::rtl::math::sin( F_PI*fZ) < 0.0) ? -1.0 : 1.0);
}


/** You must ensure fZ>0 */
double ScInterpreter::GetLogGamma(double fZ)
{
    if (fZ >= fMaxGammaArgument)
        return lcl_GetLogGammaHelper(fZ);
    if (fZ >= 1.0)
        return log(lcl_GetGammaHelper(fZ));
    if (fZ >= 0.5)
        return log( lcl_GetGammaHelper(fZ+1) / fZ);
    return lcl_GetLogGammaHelper(fZ+2) - log(fZ+1) - log(fZ);
}

double ScInterpreter::GetBetaDist(double x, double alpha, double beta)
{
    if (beta == 1.0)
        return pow(x, alpha);
    else if (alpha == 1.0)
        return 1.0 - pow(1.0-x,beta);
    double fEps = 1.0E-8;
    BOOL bReflect;
    double cf, fA, fB;
    if (x < (alpha+1.0)/(alpha+beta+1.0))
    {
        bReflect = FALSE;
        fA = alpha;
        fB = beta;
    }
    else
    {
        bReflect = TRUE;
        fA = beta;
        fB = alpha;
        x = 1.0 - x;
    }
    if (x < fEps)
        cf = 0.0;
    else
    {
        double a1, b1, a2, b2, fnorm, rm, apl2m, d2m, d2m1, cfnew;
        a1 = 1.0; b1 = 1.0;
        b2 = 1.0 - (fA+fB)*x/(fA+1.0);
        if (b2 == 0.0)
        {
            a2 = b2;
            fnorm = 1.0;
            cf = 1.0;
        }
        else
        {
            a2 = 1.0;
            fnorm = 1.0/b2;
            cf = a2*fnorm;
        }
        cfnew = 1.0;
        for (USHORT j = 1; j <= 100; j++)
        {
            rm = (double) j;
            apl2m = fA + 2.0*rm;
            d2m = rm*(fB-rm)*x/((apl2m-1.0)*apl2m);
            d2m1 = -(fA+rm)*(fA+fB+rm)*x/(apl2m*(apl2m+1.0));
            a1 = (a2+d2m*a1)*fnorm;
            b1 = (b2+d2m*b1)*fnorm;
            a2 = a1 + d2m1*a2*fnorm;
            b2 = b1 + d2m1*b2*fnorm;
            if (b2 != 0.0)
            {
                fnorm = 1.0/b2;
                cfnew = a2*fnorm;
                if (fabs(cf-cfnew)/cf < fEps)
                    j = 101;
                else
                    cf = cfnew;
            }
        }
        if (fB < fEps)
            b1 = 69;            // ln(1.0E30)
        else
            b1 = GetLogGamma(fA)+GetLogGamma(fB)-GetLogGamma(fA+fB);

        // cf *= pow(x, fA)*pow(1.0-x,fB)/(fA*exp(b1));
        // #108995# The formula above has 0 as results for the terms too easily,
        // resulting in an error where the equivalent formula below still works:
        // (x can't be 0 or 1, this is handled above)
        cf *= exp( log(x)*fA + log(1.0-x)*fB - b1 ) / fA;
    }
    if (bReflect)
        return 1.0-cf;
    else
        return cf;
}

double ScInterpreter::GetFDist(double x, double fF1, double fF2)
{
    double arg = fF2/(fF2+fF1*x);
    double alpha = fF2/2.0;
    double beta = fF1/2.0;
    return (GetBetaDist(arg, alpha, beta));
/*
    double Z = (pow(fF,1.0/3.0)*(1.0-2.0/(9.0*fF2)) - (1.0-2.0/(9.0*fF1))) /
               sqrt(2.0/(9.0*fF1) + pow(fF,2.0/3.0)*2.0/(9.0*fF2));
    return (0.5-gauss(Z));
*/
}

double ScInterpreter::GetTDist(double T, double fDF)
{
    return 0.5 * GetBetaDist(fDF/(fDF+T*T), fDF/2.0, 0.5);
/*
    USHORT DF = (USHORT) fDF;
    double A = T / sqrt(DF);
    double B = 1.0 + A*A;
    double R;
    if (DF == 1)
        R = 0.5 + atan(A)/F_PI;
    else if (DF % 2 == 0)
    {
        double S0 = A/(2.0 * sqrt(B));
        double C0 = S0;
        for (USHORT i = 2; i <= DF-2; i+=2)
        {
            C0 *= (1.0 - 1.0/(double)i)/B;
            S0 += C0;
        }
        R = 0.5 + S0;
    }
    else
    {
        double S1 = A / (B * F_PI);
        double C1 = S1;
        for (USHORT i = 3; i <= DF-2; i+=2)
        {
            C1 *= (1.0 - 1.0/(double)i)/B;
            S1 += C1;
        }
        R = 0.5 + atan(A)/F_PI + S1;
    }
    return 1.0 - R;
*/
}

// for LEGACY.CHIDIST, returns right tail, fDF=degrees of freedom
/** You must ensure fDF>0.0 */
double ScInterpreter::GetChiDist(double fX, double fDF)
{
    if (fX <= 0.0)
        return 1.0; // see ODFF
    else
        return GetUpRegIGamma( fDF/2.0, fX/2.0);
}

// ready for ODF 1.2
// for ODF CHISQDIST; cumulative distribution function, fDF=degrees of freedom
// returns left tail
/** You must ensure fDF>0.0 */
double ScInterpreter::GetChiSqDistCDF(double fX, double fDF)
{
    if (fX <= 0.0)
        return 0.0; // see ODFF
    else
        return GetLowRegIGamma( fDF/2.0, fX/2.0);
}

// for ODF 1.2 GAMMA
void ScInterpreter::ScGamma()
{
    double x = GetDouble();
    double fResult;
    if (x <= 0.0 && x == ::rtl::math::approxFloor(x))
        PushIllegalArgument();
    else
    {
        fResult = GetGamma(x);
        if (nGlobalError)
        {
            PushError( nGlobalError);
            return;
        }
        PushDouble(fResult);
    }
}


void ScInterpreter::ScLogGamma()
{
    double x = GetDouble();
    if (x > 0.0)    // constraint from ODFF
        PushDouble( GetLogGamma(x));
    else
        PushIllegalArgument();
}


void ScInterpreter::ScBetaDist()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    double fA, fB, alpha, beta, x;
    if (nParamCount == 5)
        fB = GetDouble();
    else
        fB = 1.0;
    if (nParamCount >= 4)
        fA = GetDouble();
    else
        fA = 0.0;
    beta = GetDouble();
    alpha = GetDouble();
    x = GetDouble();
    if (x < fA || x > fB || fA == fB || alpha <= 0.0 || beta <= 0.0)
    {
        PushIllegalArgument();
        return;
    }
    x = (x-fA)/(fB-fA);                 // Skalierung auf (0,1)
    PushDouble(GetBetaDist(x, alpha, beta));
}

void ScInterpreter::ScPhi()
{
    PushDouble(phi(GetDouble()));
}

void ScInterpreter::ScGauss()
{
    PushDouble(gauss(GetDouble()));
}

void ScInterpreter::ScFisher()
{
    double fVal = GetDouble();
    if (fabs(fVal) >= 1.0)
        PushIllegalArgument();
    else
        PushDouble(0.5*log((1.0+fVal)/(1.0-fVal)));
}

void ScInterpreter::ScFisherInv()
{
    PushDouble( tanh( GetDouble()));
}

void ScInterpreter::ScFact()
{
    double nVal = GetDouble();
    if (nVal < 0.0)
        PushIllegalArgument();
    else
        PushDouble(Fakultaet(nVal));
}

void ScInterpreter::ScKombin()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double k = ::rtl::math::approxFloor(GetDouble());
        double n = ::rtl::math::approxFloor(GetDouble());
        if (k < 0.0 || n < 0.0 || k > n)
            PushIllegalArgument();
        else
            PushDouble(BinomKoeff(n, k));
    }
}

void ScInterpreter::ScKombin2()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double k = ::rtl::math::approxFloor(GetDouble());
        double n = ::rtl::math::approxFloor(GetDouble());
        if (k < 0.0 || n < 0.0 || k > n)
            PushIllegalArgument();
        else
            PushDouble(BinomKoeff(n + k - 1, k));
    }
}

void ScInterpreter::ScVariationen()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double k = ::rtl::math::approxFloor(GetDouble());
        double n = ::rtl::math::approxFloor(GetDouble());
        if (n < 0.0 || k < 0.0 || k > n)
            PushIllegalArgument();
        else if (k == 0.0)
            PushInt(1);     // (n! / (n - 0)!) == 1
        else
        {
            double nVal = n;
            for (ULONG i = (ULONG)k-1; i >= 1; i--)
                nVal *= n-(double)i;
            PushDouble(nVal);
        }
    }
}

void ScInterpreter::ScVariationen2()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double k = ::rtl::math::approxFloor(GetDouble());
        double n = ::rtl::math::approxFloor(GetDouble());
        if (n < 0.0 || k < 0.0 || k > n)
            PushIllegalArgument();
        else
            PushDouble(pow(n,k));
    }
}

void ScInterpreter::ScB()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return ;
    if (nParamCount == 3)
    {
        double x = ::rtl::math::approxFloor(GetDouble());
        double p = GetDouble();
        double n = ::rtl::math::approxFloor(GetDouble());
        if (n < 0.0 || x < 0.0 || x > n || p < 0.0 || p > 1.0)
            PushIllegalArgument();
        else
        {
            double q = 1.0 - p;
            double fFactor = pow(q, n);
            if (fFactor == 0.0)
            {
                fFactor = pow(p, n);
                if (fFactor == 0.0)
                    PushNoValue();
                else
                {
                    ULONG max = (ULONG) (n - x);
                    for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                        fFactor *= (n-i)/(i+1)*q/p;
                    PushDouble(fFactor);
                }
            }
            else
            {
                ULONG max = (ULONG) x;
                for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                    fFactor *= (n-i)/(i+1)*p/q;
                PushDouble(fFactor);
            }
        }
    }
    else if (nParamCount == 4)
    {
        double xe = GetDouble();
        double xs = GetDouble();
        double p = GetDouble();
        double n = GetDouble();
//                                          alter Stand 300-SC
//      if ((xs < n) && (xe < n) && (p < 1.0))
//      {
//          double Varianz = sqrt(n * p * (1.0 - p));
//          xs = fabs(xs - (n * p /* / 2.0 STE */ ));
//          xe = fabs(xe - (n * p /* / 2.0 STE */ ));
//// STE        double nVal = gauss((xs + 0.5) / Varianz) + gauss((xe + 0.5) / Varianz);
//          double nVal = fabs(gauss(xs / Varianz) - gauss(xe / Varianz));
//          PushDouble(nVal);
//      }
        if (xe <= n && xs <= xe &&
            p < 1.0 && p > 0.0 && n >= 0.0 && xs >= 0.0 )
        {
            double q = 1.0 - p;
            double fFactor = pow(q, n);
            if (fFactor == 0.0)
            {
                fFactor = pow(p, n);
                if (fFactor == 0.0)
                    PushNoValue();
                else
                {
                    double fSum = 0.0;
                    ULONG max;
                    if (xe < (ULONG) n)
                        max = (ULONG) (n-xe)-1;
                    else
                        max = 0;
                    ULONG i;
                    for (i = 0; i < max && fFactor > 0.0; i++)
                        fFactor *= (n-i)/(i+1)*q/p;
                    if (xs < (ULONG) n)
                        max = (ULONG) (n-xs);
                    else
                        fSum = fFactor;
                    for (; i < max && fFactor > 0.0; i++)
                    {
                        fFactor *= (n-i)/(i+1)*q/p;
                        fSum += fFactor;
                    }
                    PushDouble(fSum);
                }
            }
            else
            {
                ULONG max;
                double fSum;
                if ( (ULONG) xs == 0)
                {
                    fSum = fFactor;
                    max = 0;
                }
                else
                {
                    max = (ULONG) xs-1;
                    fSum = 0.0;
                }
                ULONG i;
                for (i = 0; i < max && fFactor > 0.0; i++)
                    fFactor *= (n-i)/(i+1)*p/q;
                if ((ULONG)xe == 0)                     // beide 0
                    fSum = fFactor;
                else
                    max = (ULONG) xe;
                for (; i < max && fFactor > 0.0; i++)
                {
                    fFactor *= (n-i)/(i+1)*p/q;
                    fSum += fFactor;
                }
                PushDouble(fSum);
            }
        }
        else
            PushIllegalArgument();
    }
}

void ScInterpreter::ScBinomDist()
{
    if ( MustHaveParamCount( GetByte(), 4 ) )
    {
        double kum    = GetDouble();                    // 0 oder 1
        double p      = GetDouble();                    // p
        double n      = ::rtl::math::approxFloor(GetDouble());              // n
        double x      = ::rtl::math::approxFloor(GetDouble());              // x
        double fFactor, q, fSum;
        if (n < 0.0 || x < 0.0 || x > n || p < 0.0 || p > 1.0)
            PushIllegalArgument();
        else if (kum == 0.0)                        // Dichte
        {
            q = 1.0 - p;
            fFactor = pow(q, n);
            if (fFactor == 0.0)
            {
                fFactor = pow(p, n);
                if (fFactor == 0.0)
                    PushNoValue();
                else
                {
                    ULONG max = (ULONG) (n - x);
                    for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                        fFactor *= (n-i)/(i+1)*q/p;
                    PushDouble(fFactor);
                }
            }
            else
            {
                ULONG max = (ULONG) x;
                for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                    fFactor *= (n-i)/(i+1)*p/q;
                PushDouble(fFactor);
            }
        }
        else                                        // Verteilung
        {
            if (n == x)
                PushDouble(1.0);
            else
            {
                q = 1.0 - p;
                fFactor = pow(q, n);
                if (fFactor == 0.0)
                {
                    fFactor = pow(p, n);
                    if (fFactor == 0.0)
                        PushNoValue();
                    else
                    {
                        fSum = 1.0 - fFactor;
                        ULONG max = (ULONG) (n - x) - 1;
                        for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                        {
                            fFactor *= (n-i)/(i+1)*q/p;
                            fSum -= fFactor;
                        }
                        if (fSum < 0.0)
                            PushDouble(0.0);
                        else
                            PushDouble(fSum);
                    }
                }
                else
                {
                    fSum = fFactor;
                    ULONG max = (ULONG) x;
                    for (ULONG i = 0; i < max && fFactor > 0.0; i++)
                    {
                        fFactor *= (n-i)/(i+1)*p/q;
                        fSum += fFactor;
                    }
                    PushDouble(fSum);
                }
            }
        }
    }
}

void ScInterpreter::ScCritBinom()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double alpha  = GetDouble();                    // alpha
        double p      = GetDouble();                    // p
        double n      = ::rtl::math::approxFloor(GetDouble());
        if (n < 0.0 || alpha <= 0.0 || alpha >= 1.0 || p < 0.0 || p > 1.0)
            PushIllegalArgument();
        else
        {
            double q = 1.0 - p;
            double fFactor = pow(q,n);
            if (fFactor == 0.0)
            {
                fFactor = pow(p, n);
                if (fFactor == 0.0)
                    PushNoValue();
                else
                {
                    double fSum = 1.0 - fFactor; ULONG max = (ULONG) n;
                    ULONG i;

                    for ( i = 0; i < max && fSum >= alpha; i++)
                    {
                        fFactor *= (n-i)/(i+1)*q/p;
                        fSum -= fFactor;
                    }
                    PushDouble(n-i);
                }
            }
            else
            {
                double fSum = fFactor; ULONG max = (ULONG) n;
                ULONG i;

                for ( i = 0; i < max && fSum < alpha; i++)
                {
                    fFactor *= (n-i)/(i+1)*p/q;
                    fSum += fFactor;
                }
                PushDouble(i);
            }
        }
    }
}

void ScInterpreter::ScNegBinomDist()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double p      = GetDouble();                    // p
        double r      = GetDouble();                    // r
        double x      = GetDouble();                    // x
        if (r < 0.0 || x < 0.0 || p < 0.0 || p > 1.0)
            PushIllegalArgument();
        else
        {
            double q = 1.0 - p;
            double fFactor = pow(p,r);
            for (double i = 0.0; i < x; i++)
                fFactor *= (i+r)/(i+1.0)*q;
            PushDouble(fFactor);
        }
    }
}

void ScInterpreter::ScNormDist()
{
    if ( MustHaveParamCount( GetByte(), 4 ) )
    {
        double kum = GetDouble();                   // 0 oder 1
        double sigma = GetDouble();                 // Stdabw
        double mue = GetDouble();                   // Mittelwert
        double x = GetDouble();                     // x
        if (sigma < 0.0)
            PushError( errIllegalArgument);
        else if (sigma == 0.0)
            PushError( errDivisionByZero);
        else if (kum == 0.0)                        // Dichte
            PushDouble(phi((x-mue)/sigma)/sigma);
        else                                        // Verteilung
            PushDouble(0.5 + gauss((x-mue)/sigma));
    }
}

void ScInterpreter::ScLogNormDist()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();                 // Stdabw
        double mue = GetDouble();                   // Mittelwert
        double x = GetDouble();                     // x
        if (sigma < 0.0)
            PushError( errIllegalArgument);
        else if (sigma == 0.0)
            PushError( errDivisionByZero);
        else if (x <= 0.0)
            PushIllegalArgument();
        else
            PushDouble(0.5 + gauss((log(x)-mue)/sigma));
    }
}

void ScInterpreter::ScStdNormDist()
{
    PushDouble(0.5 + gauss(GetDouble()));
}

void ScInterpreter::ScExpDist()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double kum    = GetDouble();                    // 0 oder 1
        double lambda = GetDouble();                    // lambda
        double x      = GetDouble();                    // x
        if (lambda <= 0.0)
            PushIllegalArgument();
        else if (kum == 0.0)                        // Dichte
        {
            if (x >= 0.0)
                PushDouble(lambda * exp(-lambda*x));
            else
                PushInt(0);
        }
        else                                        // Verteilung
        {
            if (x > 0.0)
                PushDouble(1.0 - exp(-lambda*x));
            else
                PushInt(0);
        }
    }
}

void ScInterpreter::ScTDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fFlag = ::rtl::math::approxFloor(GetDouble());
    double fDF   = ::rtl::math::approxFloor(GetDouble());
    double T     = GetDouble();
    if (fDF < 1.0 || T < 0.0 || (fFlag != 1.0 && fFlag != 2.0) )
    {
        PushIllegalArgument();
        return;
    }
    double R = GetTDist(T, fDF);
    if (fFlag == 1.0)
        PushDouble(R);
    else
        PushDouble(2.0*R);
}

void ScInterpreter::ScFDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fF2 = ::rtl::math::approxFloor(GetDouble());
    double fF1 = ::rtl::math::approxFloor(GetDouble());
    double fF  = GetDouble();
    if (fF < 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10)
    {
        PushIllegalArgument();
        return;
    }
    PushDouble(GetFDist(fF, fF1, fF2));
}

void ScInterpreter::ScChiDist()
{
    double fResult;
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fChi = GetDouble();
    if (fDF < 1.0 || fDF >= 1.0E5 || fChi < 0.0 )
    {
        PushIllegalArgument();
        return;
    }
    fResult = GetChiDist( fChi, fDF);
    if (nGlobalError)
    {
        PushError( nGlobalError);
        return;
    }
    PushDouble(fResult);
}

void ScInterpreter::ScWeibull()
{
    if ( MustHaveParamCount( GetByte(), 4 ) )
    {
        double kum   = GetDouble();                 // 0 oder 1
        double beta  = GetDouble();                 // beta
        double alpha = GetDouble();                 // alpha
        double x     = GetDouble();                 // x
        if (alpha <= 0.0 || beta <= 0.0 || x < 0.0)
            PushIllegalArgument();
        else if (kum == 0.0)                        // Dichte
            PushDouble(alpha/pow(beta,alpha)*pow(x,alpha-1.0)*
                       exp(-pow(x/beta,alpha)));
        else                                        // Verteilung
            PushDouble(1.0 - exp(-pow(x/beta,alpha)));
    }
}

void ScInterpreter::ScPoissonDist()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double kum    = GetDouble();                    // 0 oder 1
        double lambda = GetDouble();                    // Mittelwert
        double x      = ::rtl::math::approxFloor(GetDouble());              // x
        if (lambda < 0.0 || x < 0.0)
            PushIllegalArgument();
        else if (kum == 0.0)                            // Dichte
        {
            if (lambda == 0.0)
                PushInt(0);
            else
            {
                double fPoissonVar = 1.0;
                for ( double f = 0.0; f < x; ++f )
                    fPoissonVar *= lambda / ( f + 1.0 );
                PushDouble( fPoissonVar*exp( -lambda ) );
            }
        }
        else                                            // Verteilung
        {
            if (lambda == 0.0)
                PushInt(1);
            else
            {
                double sum = 1.0;
                double fFak = 1.0;
                ULONG nEnd = (ULONG) x;
                for (ULONG i = 1; i <= nEnd; i++)
                {
                    fFak *= (double)i;
                    sum += pow( lambda, (double)i ) / fFak;
                }
                sum *= exp(-lambda);
                PushDouble(sum);
            }
        }
    }
}

/** Local function used in the calculation of the hypergeometric distribution.
 */
void lcl_PutFactorialElements( ::std::vector< double >& cn, double fLower, double fUpper, double fBase )
{
    for ( double i = fLower; i <= fUpper; ++i )
    {
        double fVal = fBase - i;
        if ( fVal > 1.0 )
            cn.push_back( fVal );
    }
}

/** Calculates a value of the hypergeometric distribution.

    The algorithm is designed to avoid unnecessary multiplications and division
    by expanding all factorial elements (9 of them).  It is done by excluding
    those ranges that overlap in the numerator and the denominator.  This allows
    for a fast calculation for large values which would otherwise cause an overflow
    in the intermediate values.

    @author Kohei Yoshida <kohei@openoffice.org>

    @see #i47296#

 */
void ScInterpreter::ScHypGeomDist()
{
    const size_t nMaxArraySize = 500000; // arbitrary max array size

    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;

    double N = ::rtl::math::approxFloor(GetDouble());
    double M = ::rtl::math::approxFloor(GetDouble());
    double n = ::rtl::math::approxFloor(GetDouble());
    double x = ::rtl::math::approxFloor(GetDouble());

    if( (x < 0.0) || (n < x) || (M < x) || (N < n) || (N < M) || (x < n - N + M) )
    {
        PushIllegalArgument();
        return;
    }

    typedef ::std::vector< double > HypContainer;
    HypContainer cnNumer, cnDenom;

    size_t nEstContainerSize = static_cast<size_t>( x + ::std::min( n, M ) );
    size_t nMaxSize = ::std::min( cnNumer.max_size(), nMaxArraySize );
    if ( nEstContainerSize > nMaxSize )
    {
        PushNoValue();
        return;
    }
    cnNumer.reserve( nEstContainerSize + 10 );
    cnDenom.reserve( nEstContainerSize + 10 );

    // Trim coefficient C first
    double fCNumVarUpper = N - n - M + x - 1.0;
    double fCDenomVarLower = 1.0;
    if ( N - n - M + x >= M - x + 1.0 )
    {
        fCNumVarUpper = M - x - 1.0;
        fCDenomVarLower = N - n - 2.0*(M - x) + 1.0;
    }

#ifdef DBG_UTIL
    double fCNumLower = N - n - fCNumVarUpper;
#endif
    double fCDenomUpper = N - n - M + x + 1.0 - fCDenomVarLower;

    double fDNumVarLower = n - M;

    if ( n >= M + 1.0 )
    {
        if ( N - M < n + 1.0 )
        {
            // Case 1

            if ( N - n < n + 1.0 )
            {
                // no overlap
                lcl_PutFactorialElements( cnNumer, 0.0, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, N - n - 1.0, N );
            }
            else
            {
                // overlap
                DBG_ASSERT( fCNumLower < n + 1.0, "ScHypGeomDist: wrong assertion" );
                lcl_PutFactorialElements( cnNumer, N - 2.0*n, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, n - 1.0, N );
            }

            DBG_ASSERT( fCDenomUpper <= N - M, "ScHypGeomDist: wrong assertion" );

            if ( fCDenomUpper < n - x + 1.0 )
                // no overlap
                lcl_PutFactorialElements( cnNumer, 1.0, N - M - n + x, N - M + 1.0 );
            else
            {
                // overlap
                lcl_PutFactorialElements( cnNumer, 1.0, N - M - fCDenomUpper, N - M + 1.0 );

                fCDenomUpper = n - x;
                fCDenomVarLower = N - M - 2.0*(n - x) + 1.0;
            }
        }
        else
        {
            // Case 2

            if ( n > M - 1.0 )
            {
                // no overlap
                lcl_PutFactorialElements( cnNumer, 0.0, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, M - 1.0, N );
            }
            else
            {
                lcl_PutFactorialElements( cnNumer, M - n, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, n - 1.0, N );
            }

            DBG_ASSERT( fCDenomUpper <= n, "ScHypGeomDist: wrong assertion" );

            if ( fCDenomUpper < n - x + 1.0 )
                // no overlap
                lcl_PutFactorialElements( cnNumer, N - M - n + 1.0, N - M - n + x, N - M + 1.0 );
            else
            {
                lcl_PutFactorialElements( cnNumer, N - M - n + 1.0, N - M - fCDenomUpper, N - M + 1.0 );
                fCDenomUpper = n - x;
                fCDenomVarLower = N - M - 2.0*(n - x) + 1.0;
            }
        }

        DBG_ASSERT( fCDenomUpper <= M, "ScHypGeomDist: wrong assertion" );
    }
    else
    {
        if ( N - M < M + 1.0 )
        {
            // Case 3

            if ( N - n < M + 1.0 )
            {
                // No overlap
                lcl_PutFactorialElements( cnNumer, 0.0, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, N - M - 1.0, N );
            }
            else
            {
                lcl_PutFactorialElements( cnNumer, N - n - M, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, n - 1.0, N );
            }

            if ( n - x + 1.0 > fCDenomUpper )
                // No overlap
                lcl_PutFactorialElements( cnNumer, 1.0, N - M - n + x, N - M + 1.0 );
            else
            {
                // Overlap
                lcl_PutFactorialElements( cnNumer, 1.0, N - M - fCDenomUpper, N - M + 1.0 );

                fCDenomVarLower = N - M - 2.0*(n - x) + 1.0;
                fCDenomUpper = n - x;
            }
        }
        else
        {
            // Case 4

            DBG_ASSERT( M >= n - x, "ScHypGeomDist: wrong assertion" );
            DBG_ASSERT( M - x <= N - M + 1.0, "ScHypGeomDist: wrong assertion" );

            if ( N - n < N - M + 1.0 )
            {
                // No overlap
                lcl_PutFactorialElements( cnNumer, 0.0, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, M - 1.0, N );
            }
            else
            {
                // Overlap
                DBG_ASSERT( fCNumLower <= N - M + 1.0, "ScHypGeomDist: wrong assertion" );

                lcl_PutFactorialElements( cnNumer, M - n, fCNumVarUpper, N - n );
                lcl_PutFactorialElements( cnDenom, 0.0, n - 1.0, N );
            }

            if ( n - x + 1.0 > fCDenomUpper )
                // No overlap
                lcl_PutFactorialElements( cnNumer, N - 2.0*M + 1.0, N - M - n + x, N - M + 1.0 );
            else if ( M >= fCDenomUpper )
            {
                lcl_PutFactorialElements( cnNumer, N - 2.0*M + 1.0, N - M - fCDenomUpper, N - M + 1.0 );

                fCDenomUpper = n - x;
                fCDenomVarLower = N - M - 2.0*(n - x) + 1.0;
            }
            else
            {
                DBG_ASSERT( M <= fCDenomUpper, "ScHypGeomDist: wrong assertion" );
                lcl_PutFactorialElements( cnDenom, fCDenomVarLower, N - n - 2.0*M + x,
                        N - n - M + x + 1.0 );

                fCDenomUpper = n - x;
                fCDenomVarLower = N - M - 2.0*(n - x) + 1.0;
            }
        }

        DBG_ASSERT( fCDenomUpper <= n, "ScHypGeomDist: wrong assertion" );

        fDNumVarLower = 0.0;
    }

    double nDNumVarUpper   = fCDenomUpper < x + 1.0 ? n - x - 1.0     : n - fCDenomUpper - 1.0;
    double nDDenomVarLower = fCDenomUpper < x + 1.0 ? fCDenomVarLower : N - n - M + 1.0;
    lcl_PutFactorialElements( cnNumer, fDNumVarLower, nDNumVarUpper, n );
    lcl_PutFactorialElements( cnDenom, nDDenomVarLower, N - n - M + x, N - n - M + x + 1.0 );

    ::std::sort( cnNumer.begin(), cnNumer.end() );
    ::std::sort( cnDenom.begin(), cnDenom.end() );
    HypContainer::reverse_iterator it1 = cnNumer.rbegin(), it1End = cnNumer.rend();
    HypContainer::reverse_iterator it2 = cnDenom.rbegin(), it2End = cnDenom.rend();

    double fFactor = 1.0;
    for ( ; it1 != it1End || it2 != it2End; )
    {
        double fEnum = 1.0, fDenom = 1.0;
        if ( it1 != it1End )
            fEnum  = *it1++;
        if ( it2 != it2End )
            fDenom = *it2++;
        fFactor *= fEnum / fDenom;
    }

    PushDouble(fFactor);
}

void ScInterpreter::ScGammaDist()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;
    double fCumulative = GetDouble(); // parameter type will change to bool
    bool bCumulative = (fCumulative == 1.0);    // see ODFF
    double fBeta = GetDouble();                 // scale
    double fAlpha = GetDouble();                // shape
    double fX = GetDouble();                    // x
    if (fAlpha <= 0.0 || fBeta <= 0.0)
        PushIllegalArgument();
    else
    {
        if (bCumulative)                        // distribution
            PushDouble( GetGammaDist( fX, fAlpha, fBeta));
        else                                    // density
            PushDouble( GetGammaDistPDF( fX, fAlpha, fBeta));
    }
}

void ScInterpreter::ScNormInv()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();
        double mue   = GetDouble();
        double x     = GetDouble();
        if (sigma <= 0.0 || x < 0.0 || x > 1.0)
            PushIllegalArgument();
        else if (x == 0.0 || x == 1.0)
            PushNoValue();
        else
            PushDouble(gaussinv(x)*sigma + mue);
    }
}

void ScInterpreter::ScSNormInv()
{
    double x = GetDouble();
    if (x < 0.0 || x > 1.0)
        PushIllegalArgument();
    else if (x == 0.0 || x == 1.0)
        PushNoValue();
    else
        PushDouble(gaussinv(x));
}

void ScInterpreter::ScLogNormInv()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();                 // Stdabw
        double mue = GetDouble();                   // Mittelwert
        double y = GetDouble();                     // y
        if (sigma <= 0.0 || y <= 0.0 || y >= 1.0)
            PushIllegalArgument();
        else
            PushDouble(exp(mue+sigma*gaussinv(y)));
    }
}

class ScGammaDistFunction : public ScDistFunc
{
    ScInterpreter&  rInt;
    double          fp, fAlpha, fBeta;

public:
            ScGammaDistFunction( ScInterpreter& rI, double fpVal, double fAlphaVal, double fBetaVal ) :
                rInt(rI), fp(fpVal), fAlpha(fAlphaVal), fBeta(fBetaVal) {}

    double  GetValue( double x ) const  { return fp - rInt.GetGammaDist(x, fAlpha, fBeta); }
};

void ScInterpreter::ScGammaInv()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fBeta  = GetDouble();
    double fAlpha = GetDouble();
    double fP = GetDouble();
    if (fAlpha <= 0.0 || fBeta <= 0.0 || fP < 0.0 || fP >= 1.0 )
    {
        PushIllegalArgument();
        return;
    }
    if (fP == 0.0)
        PushInt(0);
    else
    {
        bool bConvError;
        ScGammaDistFunction aFunc( *this, fP, fAlpha, fBeta );
        double fStart = fAlpha * fBeta;
        double fVal = lcl_IterateInverse( aFunc, fStart*0.5, fStart, bConvError );
        if (bConvError)
            SetError(errNoConvergence);
        PushDouble(fVal);
    }
}

class ScBetaDistFunction : public ScDistFunc
{
    ScInterpreter&  rInt;
    double          fp, fAlpha, fBeta;

public:
            ScBetaDistFunction( ScInterpreter& rI, double fpVal, double fAlphaVal, double fBetaVal ) :
                rInt(rI), fp(fpVal), fAlpha(fAlphaVal), fBeta(fBetaVal) {}

    double  GetValue( double x ) const  { return fp - rInt.GetBetaDist(x, fAlpha, fBeta); }
};

void ScInterpreter::ScBetaInv()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    double fP, fA, fB, fAlpha, fBeta;
    if (nParamCount == 5)
        fB = GetDouble();
    else
        fB = 1.0;
    if (nParamCount >= 4)
        fA = GetDouble();
    else
        fA = 0.0;
    fBeta  = GetDouble();
    fAlpha = GetDouble();
    fP     = GetDouble();
    if (fP < 0.0 || fP >= 1.0 || fA == fB || fAlpha <= 0.0 || fBeta <= 0.0)
    {
        PushIllegalArgument();
        return;
    }
    if (fP == 0.0)
        PushInt(0);
    else
    {
        bool bConvError;
        ScBetaDistFunction aFunc( *this, fP, fAlpha, fBeta );
        // 0..1 as range for iteration so it isn't extended beyond the valid range
        double fVal = lcl_IterateInverse( aFunc, 0.0, 1.0, bConvError );
        if (bConvError)
            PushError( errNoConvergence);
        else
            PushDouble(fA + fVal*(fB-fA));                  // scale to (A,B)
    }
}

                                                            // Achtung: T, F und Chi
                                                            // sind monoton fallend,
                                                            // deshalb 1-Dist als Funktion

class ScTDistFunction : public ScDistFunc
{
    ScInterpreter&  rInt;
    double          fp, fDF;

public:
            ScTDistFunction( ScInterpreter& rI, double fpVal, double fDFVal ) :
                rInt(rI), fp(fpVal), fDF(fDFVal) {}

    double  GetValue( double x ) const  { return fp - 2 * rInt.GetTDist(x, fDF); }
};

void ScInterpreter::ScTInv()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fP = GetDouble();
    if (fDF < 1.0 || fDF >= 1.0E5 || fP <= 0.0 || fP > 1.0 )
    {
        PushIllegalArgument();
        return;
    }

    bool bConvError;
    ScTDistFunction aFunc( *this, fP, fDF );
    double fVal = lcl_IterateInverse( aFunc, fDF*0.5, fDF, bConvError );
    if (bConvError)
        SetError(errNoConvergence);
    PushDouble(fVal);
}

class ScFDistFunction : public ScDistFunc
{
    ScInterpreter&  rInt;
    double          fp, fF1, fF2;

public:
            ScFDistFunction( ScInterpreter& rI, double fpVal, double fF1Val, double fF2Val ) :
                rInt(rI), fp(fpVal), fF1(fF1Val), fF2(fF2Val) {}

    double  GetValue( double x ) const  { return fp - rInt.GetFDist(x, fF1, fF2); }
};

void ScInterpreter::ScFInv()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fF2 = ::rtl::math::approxFloor(GetDouble());
    double fF1 = ::rtl::math::approxFloor(GetDouble());
    double fP  = GetDouble();
    if (fP <= 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10 || fP > 1.0)
    {
        PushIllegalArgument();
        return;
    }

    bool bConvError;
    ScFDistFunction aFunc( *this, fP, fF1, fF2 );
    double fVal = lcl_IterateInverse( aFunc, fF1*0.5, fF1, bConvError );
    if (bConvError)
        SetError(errNoConvergence);
    PushDouble(fVal);
}

class ScChiDistFunction : public ScDistFunc
{
    ScInterpreter&  rInt;
    double          fp, fDF;

public:
            ScChiDistFunction( ScInterpreter& rI, double fpVal, double fDFVal ) :
                rInt(rI), fp(fpVal), fDF(fDFVal) {}

    double  GetValue( double x ) const  { return fp - rInt.GetChiDist(x, fDF); }
};

void ScInterpreter::ScChiInv()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fP = GetDouble();
    if (fDF < 1.0 || fDF >= 1.0E5 || fP <= 0.0 || fP > 1.0 )
    {
        PushIllegalArgument();
        return;
    }

    bool bConvError;
    ScChiDistFunction aFunc( *this, fP, fDF );
    double fVal = lcl_IterateInverse( aFunc, fDF*0.5, fDF, bConvError );
    if (bConvError)
        SetError(errNoConvergence);
    PushDouble(fVal);
}

/***********************************************/

void ScInterpreter::ScConfidence()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double n     = ::rtl::math::approxFloor(GetDouble());
        double sigma = GetDouble();
        double alpha = GetDouble();
        if (sigma <= 0.0 || alpha <= 0.0 || alpha >= 1.0 || n < 1.0)
            PushIllegalArgument();
        else
            PushDouble( gaussinv(1.0-alpha/2.0) * sigma/sqrt(n) );
    }
}

void ScInterpreter::ScZTest()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    double sigma = 0.0, mue, x;
    if (nParamCount == 3)
    {
        sigma = GetDouble();
        if (sigma <= 0.0)
        {
            PushIllegalArgument();
            return;
        }
    }
    x = GetDouble();

    double fSum    = 0.0;
    double fSumSqr = 0.0;
    double fVal;
    double rValCount = 0.0;
    switch (GetStackType())
    {
        case svDouble :
        {
            fVal = GetDouble();
            fSum    += fVal;
            fSumSqr += fVal*fVal;
            rValCount++;
        }
        break;
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            ScBaseCell* pCell = GetCell( aAdr );
            if (HasCellValueData(pCell))
            {
                fVal = GetCellValue( aAdr, pCell );
                fSum += fVal;
                fSumSqr += fVal*fVal;
                rValCount++;
            }
        }
        break;
        case svRefList :
        case svDoubleRef :
        {
            short nParam = 1;
            size_t nRefInList = 0;
            while (nParam-- > 0)
            {
                ScRange aRange;
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter(pDok, aRange, glSubTotal);
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    fSumSqr += fVal*fVal;
                    rValCount++;
                    while ((nErr == 0) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        fSumSqr += fVal*fVal;
                        rValCount++;
                    }
                    SetError(nErr);
                }
            }
        }
        break;
        case svMatrix :
        {
            ScMatrixRef pMat = PopMatrix();
            if (pMat)
            {
                SCSIZE nCount = pMat->GetElementCount();
                if (pMat->IsNumeric())
                {
                    for ( SCSIZE i = 0; i < nCount; i++ )
                    {
                        fVal= pMat->GetDouble(i);
                        fSum += fVal;
                        fSumSqr += fVal * fVal;
                        rValCount++;
                    }
                }
                else
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                        if (!pMat->IsString(i))
                        {
                            fVal= pMat->GetDouble(i);
                            fSum += fVal;
                            fSumSqr += fVal * fVal;
                            rValCount++;
                        }
                }
            }
        }
        break;
        default : SetError(errIllegalParameter); break;
    }
    if (rValCount <= 1.0)
        PushError( errDivisionByZero);
    else
    {
        mue = fSum/rValCount;
        if (nParamCount != 3)
            sigma = (fSumSqr - fSum*fSum/rValCount)/(rValCount-1.0);

        PushDouble(0.5 - gauss((mue-x)/sqrt(sigma/rValCount)));
    }
}

void ScInterpreter::ScTTest()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;
    double fTyp = ::rtl::math::approxFloor(GetDouble());
    double fAnz = ::rtl::math::approxFloor(GetDouble());
    if (fAnz != 1.0 && fAnz != 2.0)
    {
        PushIllegalArgument();
        return;
    }

    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    double fT, fF;
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    SCSIZE i, j;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (fTyp == 1.0)
    {
        if (nC1 != nC2 || nR1 != nR2)
        {
            PushIllegalArgument();
            return;
        }
        double fCount   = 0.0;
        double fSum1    = 0.0;
        double fSum2    = 0.0;
        double fSumSqrD = 0.0;
        double fVal1, fVal2;
        for (i = 0; i < nC1; i++)
            for (j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    fVal1 = pMat1->GetDouble(i,j);
                    fVal2 = pMat2->GetDouble(i,j);
                    fSum1    += fVal1;
                    fSum2    += fVal2;
                    fSumSqrD += (fVal1 - fVal2)*(fVal1 - fVal2);
                    fCount++;
                }
            }
        if (fCount < 1.0)
        {
            PushNoValue();
            return;
        }
        fT = sqrt(fCount-1.0) * fabs(fSum1 - fSum2) /
             sqrt(fCount * fSumSqrD - (fSum1-fSum2)*(fSum1-fSum2));
        fF = fCount - 1.0;
    }
    else if (fTyp == 2.0)
    {
        double fCount1  = 0.0;
        double fCount2  = 0.0;
        double fSum1    = 0.0;
        double fSumSqr1 = 0.0;
        double fSum2    = 0.0;
        double fSumSqr2 = 0.0;
        double fVal;
        for (i = 0; i < nC1; i++)
            for (j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j))
                {
                    fVal = pMat1->GetDouble(i,j);
                    fSum1    += fVal;
                    fSumSqr1 += fVal * fVal;
                    fCount1++;
                }
            }
        for (i = 0; i < nC2; i++)
            for (j = 0; j < nR2; j++)
            {
                if (!pMat2->IsString(i,j))
                {
                    fVal = pMat2->GetDouble(i,j);
                    fSum2    += fVal;
                    fSumSqr2 += fVal * fVal;
                    fCount2++;
                }
            }
        if (fCount1 < 2.0 || fCount2 < 2.0)
        {
            PushNoValue();
            return;
        }
#if 0
        //  alter Templin-Code
        double fS1 = (fSumSqr1-fSum1*fSum1/fCount1)/(fCount1-1.0)/fCount1;
        double fS2 = (fSumSqr2-fSum2*fSum2/fCount2)/(fCount2-1.0)/fCount2;
        if (fS1 + fS2 == 0.0)
        {
            PushNoValue();
            return;
        }
        fT = fabs(fSum1/fCount1 - fSum2/fCount2)/sqrt(fS1+fS2);
        fF = fCount1 + fCount2 - 2;
#else
        //  laut Bronstein-Semendjajew
        double fS1 = (fSumSqr1 - fSum1*fSum1/fCount1) / (fCount1 - 1.0);    // Varianz
        double fS2 = (fSumSqr2 - fSum2*fSum2/fCount2) / (fCount2 - 1.0);
        fT = fabs( fSum1/fCount1 - fSum2/fCount2 ) /
             sqrt( (fCount1-1.0)*fS1 + (fCount2-1.0)*fS2 ) *
             sqrt( fCount1*fCount2*(fCount1+fCount2-2)/(fCount1+fCount2) );
        fF = fCount1 + fCount2 - 2;
#endif
    }
    else if (fTyp == 3.0)
    {
        double fCount1  = 0.0;
        double fCount2  = 0.0;
        double fSum1    = 0.0;
        double fSumSqr1 = 0.0;
        double fSum2    = 0.0;
        double fSumSqr2 = 0.0;
        double fVal;
        for (i = 0; i < nC1; i++)
            for (j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j))
                {
                    fVal = pMat1->GetDouble(i,j);
                    fSum1    += fVal;
                    fSumSqr1 += fVal * fVal;
                    fCount1++;
                }
            }
        for (i = 0; i < nC2; i++)
            for (j = 0; j < nR2; j++)
            {
                if (!pMat2->IsString(i,j))
                {
                    fVal = pMat2->GetDouble(i,j);
                    fSum2    += fVal;
                    fSumSqr2 += fVal * fVal;
                    fCount2++;
                }
            }
        if (fCount1 < 2.0 || fCount2 < 2.0)
        {
            PushNoValue();
            return;
        }
        double fS1 = (fSumSqr1-fSum1*fSum1/fCount1)/(fCount1-1.0)/fCount1;
        double fS2 = (fSumSqr2-fSum2*fSum2/fCount2)/(fCount2-1.0)/fCount2;
        if (fS1 + fS2 == 0.0)
        {
            PushNoValue();
            return;
        }
        fT = fabs(fSum1/fCount1 - fSum2/fCount2)/sqrt(fS1+fS2);
        double c = fS1/(fS1+fS2);
// s.u. fF = ::rtl::math::approxFloor(1.0/(c*c/(fCount1-1.0)+(1.0-c)*(1.0-c)/(fCount2-1.0)));
//      fF = ::rtl::math::approxFloor((fS1+fS2)*(fS1+fS2)/(fS1*fS1/(fCount1-1.0) + fS2*fS2/(fCount2-1.0)));

        //  GetTDist wird mit GetBetaDist berechnet und kommt auch mit nicht ganzzahligen
        //  Freiheitsgraden klar. Dann stimmt das Ergebnis auch mit Excel ueberein (#52406#):
        fF = 1.0/(c*c/(fCount1-1.0)+(1.0-c)*(1.0-c)/(fCount2-1.0));
    }

    else
    {
        PushIllegalArgument();
        return;
    }
    if (fAnz == 1.0)
        PushDouble(GetTDist(fT, fF));
    else
        PushDouble(2.0*GetTDist(fT, fF));
}

void ScInterpreter::ScFTest()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    SCSIZE i, j;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    double fCount1  = 0.0;
    double fCount2  = 0.0;
    double fSum1    = 0.0;
    double fSumSqr1 = 0.0;
    double fSum2    = 0.0;
    double fSumSqr2 = 0.0;
    double fVal;
    for (i = 0; i < nC1; i++)
        for (j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j))
            {
                fVal = pMat1->GetDouble(i,j);
                fSum1    += fVal;
                fSumSqr1 += fVal * fVal;
                fCount1++;
            }
        }
    for (i = 0; i < nC2; i++)
        for (j = 0; j < nR2; j++)
        {
            if (!pMat2->IsString(i,j))
            {
                fVal = pMat2->GetDouble(i,j);
                fSum2    += fVal;
                fSumSqr2 += fVal * fVal;
                fCount2++;
            }
        }
    if (fCount1 < 2.0 || fCount2 < 2.0)
    {
        PushNoValue();
        return;
    }
    double fS1 = (fSumSqr1-fSum1*fSum1/fCount1)/(fCount1-1.0);
    double fS2 = (fSumSqr2-fSum2*fSum2/fCount2)/(fCount2-1.0);
    if (fS1 == 0.0 || fS2 == 0.0)
    {
        PushNoValue();
        return;
    }
    double fF, fF1, fF2;
    if (fS1 > fS2)
    {
        fF = fS1/fS2;
        fF1 = fCount1-1.0;
        fF2 = fCount2-1.0;
    }
    else
    {
        fF = fS2/fS1;
        fF1 = fCount2-1.0;
        fF2 = fCount1-1.0;
    }
    PushDouble(2.0*GetFDist(fF, fF1, fF2));
/*
    double Z = (pow(fF,1.0/3.0)*(1.0-2.0/(9.0*fF2)) - (1.0-2.0/(9.0*fF1))) /
               sqrt(2.0/(9.0*fF1) + pow(fF,2.0/3.0)*2.0/(9.0*fF2));
    PushDouble(1.0-2.0*gauss(Z));
*/
}

void ScInterpreter::ScChiTest()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    double fChi = 0.0;
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValE = pMat2->GetDouble(i,j);
                fChi += (fValX - fValE) * (fValX - fValE) / fValE;
            }
            else
            {
                PushIllegalArgument();
                return;
            }
        }
    }
    double fDF;
    if (nC1 == 1 || nR1 == 1)
    {
        fDF = (double)(nC1*nR1 - 1);
        if (fDF == 0.0)
        {
            PushNoValue();
            return;
        }
    }
    else
        fDF = (double)(nC1-1)*(double)(nR1-1);
    PushDouble(GetChiDist(fChi, fDF));
/*
    double fX, fS, fT, fG;
    fX = 1.0;
    for (double fi = fDF; fi >= 2.0; fi -= 2.0)
        fX *= fChi/fi;
    fX *= exp(-fChi/2.0);
    if (fmod(fDF, 2.0) != 0.0)
        fX *= sqrt(2.0*fChi/F_PI);
    fS = 1.0;
    fT = 1.0;
    fG = fDF;
    while (fT >= 1.0E-7)
    {
        fG += 2.0;
        fT *= fChi/fG;
        fS += fT;
    }
    PushDouble(1.0 - fX*fS);
*/
}

void ScInterpreter::ScKurt()
{
    short nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    double fSum    = 0.0;
    double vSum    = 0.0;
    std::vector<double> values;
    double fCount  = 0.0;
    double fVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                fVal = GetDouble();
                fSum += fVal;
                values.push_back(fVal);
                fCount++;
            }
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    fVal = GetCellValue( aAdr, pCell );
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                PopDoubleRef( aRange, nParamCount, nRefInList);
                USHORT nErr = 0;
                ScValueIterator aValIter(pDok, aRange);
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        values.push_back(fVal);
                        fCount++;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            fVal = pMat->GetDouble(nElem);
                            fSum += fVal;
                            values.push_back(fVal);
                            fCount++;
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsString(nElem))
                            {
                                fVal = pMat->GetDouble(nElem);
                                fSum += fVal;
                                values.push_back(fVal);
                                fCount++;
                            }
                    }
                }
            }
            break;
            default :
                SetError(errIllegalParameter);
            break;
        }
    }

    if (nGlobalError)
    {
        PushError( nGlobalError);
        return;
    }

    if (fCount == 0.0)
    {
        PushError( errDivisionByZero);
        return;
    }

    double fMean = fSum / fCount;

    for (size_t i = 0; i < values.size(); i++)
        vSum += (values[i] - fMean) * (values[i] - fMean);

    double fStdDev = sqrt(vSum / (fCount - 1.0));
    double dx = 0.0;
    double xpower4 = 0.0;

    if (fStdDev == 0.0)
    {
        PushError( errDivisionByZero);
        return;
    }

    for (size_t i = 0; i < values.size(); i++)
    {
        dx = (values[i] - fMean) / fStdDev;
        xpower4 = xpower4 + (dx * dx * dx * dx);
    }

    double k_d = (fCount - 2.0) * (fCount - 3.0);
    double k_l = fCount * (fCount + 1.0) / ((fCount - 1.0) * k_d);
    double k_t = 3.0 * (fCount - 1.0) * (fCount - 1.0) / k_d;

    PushDouble(xpower4 * k_l - k_t);
}

void ScInterpreter::ScHarMean()
{
    short nParamCount = GetByte();
    double nVal = 0.0;
    double nValCount = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while ((nGlobalError == 0) && (nParamCount-- > 0))
    {
        switch (GetStackType())
        {
            case svDouble    :
            {
                double x = GetDouble();
                if (x > 0.0)
                {
                    nVal += 1.0/x;
                    nValCount++;
                }
                else
                    SetError( errIllegalArgument);
                break;
            }
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    double x = GetCellValue( aAdr, pCell );
                    if (x > 0.0)
                    {
                        nVal += 1.0/x;
                        nValCount++;
                    }
                    else
                        SetError( errIllegalArgument);
                }
                break;
            }
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                double nCellVal;
                ScValueIterator aValIter(pDok, aRange, glSubTotal);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    if (nCellVal > 0.0)
                    {
                        nVal += 1.0/nCellVal;
                        nValCount++;
                    }
                    else
                        SetError( errIllegalArgument);
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                        {
                            nVal += 1.0/nCellVal;
                            nValCount++;
                        }
                        else
                            SetError( errIllegalArgument);
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            double x = pMat->GetDouble(nElem);
                            if (x > 0.0)
                            {
                                nVal += 1.0/x;
                                nValCount++;
                            }
                            else
                                SetError( errIllegalArgument);
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsString(nElem))
                            {
                                double x = pMat->GetDouble(nElem);
                                if (x > 0.0)
                                {
                                    nVal += 1.0/x;
                                    nValCount++;
                                }
                                else
                                    SetError( errIllegalArgument);
                            }
                    }
                }
            }
            break;
            default : SetError(errIllegalParameter); break;
        }
    }
    if (nGlobalError == 0)
        PushDouble((double)nValCount/nVal);
    else
        PushError( nGlobalError);
}

void ScInterpreter::ScGeoMean()
{
    short nParamCount = GetByte();
    double nVal = 0.0;
    double nValCount = 0.0;
    ScAddress aAdr;
    ScRange aRange;

    size_t nRefInList = 0;
    while ((nGlobalError == 0) && (nParamCount-- > 0))
    {
        switch (GetStackType())
        {
            case svDouble    :
            {
                double x = GetDouble();
                if (x > 0.0)
                {
                    nVal += log(x);
                    nValCount++;
                }
                else
                    SetError( errIllegalArgument);
                break;
            }
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    double x = GetCellValue( aAdr, pCell );
                    if (x > 0.0)
                    {
                        nVal += log(x);
                        nValCount++;
                    }
                    else
                        SetError( errIllegalArgument);
                }
                break;
            }
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                double nCellVal;
                ScValueIterator aValIter(pDok, aRange, glSubTotal);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    if (nCellVal > 0.0)
                    {
                        nVal += log(nCellVal);
                        nValCount++;
                    }
                    else
                        SetError( errIllegalArgument);
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                        {
                            nVal += log(nCellVal);
                            nValCount++;
                        }
                        else
                            SetError( errIllegalArgument);
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE ui = 0; ui < nCount; ui++)
                        {
                            double x = pMat->GetDouble(ui);
                            if (x > 0.0)
                            {
                                nVal += log(x);
                                nValCount++;
                            }
                            else
                                SetError( errIllegalArgument);
                        }
                    }
                    else
                    {
                        for (SCSIZE ui = 0; ui < nCount; ui++)
                            if (!pMat->IsString(ui))
                            {
                                double x = pMat->GetDouble(ui);
                                if (x > 0.0)
                                {
                                    nVal += log(x);
                                    nValCount++;
                                }
                                else
                                    SetError( errIllegalArgument);
                            }
                    }
                }
            }
            break;
            default : SetError(errIllegalParameter); break;
        }
    }
    if (nGlobalError == 0)
        PushDouble(exp(nVal / nValCount));
    else
        PushError( nGlobalError);
}

void ScInterpreter::ScStandard()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();
        double mue   = GetDouble();
        double x     = GetDouble();
        if (sigma < 0.0)
            PushError( errIllegalArgument);
        else if (sigma == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble((x-mue)/sigma);
    }
}

void ScInterpreter::ScSkew()
{
    short nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 )  )
        return;
    double fSum    = 0.0;
    double vSum    = 0.0;
    std::vector<double> values;
    double fCount  = 0.0;
    double fVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                fVal = GetDouble();
                fSum += fVal;
                values.push_back(fVal);
                fCount++;
            }
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    fVal = GetCellValue( aAdr, pCell );
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                PopDoubleRef( aRange, nParamCount, nRefInList);
                USHORT nErr = 0;
                ScValueIterator aValIter(pDok, aRange);
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        values.push_back(fVal);
                        fCount++;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            fVal = pMat->GetDouble(nElem);
                            fSum += fVal;
                            values.push_back(fVal);
                            fCount++;
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsString(nElem))
                            {
                                fVal = pMat->GetDouble(nElem);
                                fSum += fVal;
                                values.push_back(fVal);
                                fCount++;
                            }
                    }
                }
            }
            break;
            default :
                SetError(errIllegalParameter);
            break;
        }
    }

    if (nGlobalError)
    {
        PushError( nGlobalError);
        return;
    }

    double fMean = fSum / fCount;

    for (size_t i = 0; i < values.size(); i++)
        vSum += (values[i] - fMean) * (values[i] - fMean);

    double fStdDev = sqrt(vSum / (fCount - 1.0));
    double dx = 0.0;
    double xcube = 0.0;

    if (fStdDev == 0)
    {
        PushIllegalArgument();
        return;
    }

    for (size_t i = 0; i < values.size(); i++)
    {
        dx = (values[i] - fMean) / fStdDev;
        xcube = xcube + (dx * dx * dx);
    }

    PushDouble(((xcube * fCount) / (fCount - 1.0)) / (fCount - 2.0));
}

double ScInterpreter::GetMedian( vector<double> & rArray )
{   
    size_t nSize = rArray.size();
    if (rArray.empty() || nSize == 0 || nGlobalError)
    {
        SetError( errNoValue);
        return 0.0;
    }
    
    // Upper median.
    size_t nMid = nSize / 2;
    vector<double>::iterator iMid = rArray.begin() + nMid;
    ::std::nth_element( rArray.begin(), iMid, rArray.end());
    if (nSize & 1) 
        return *iMid;   // Lower and upper median are equal.
    else
    {
        double fUp = *iMid;
        // Lower median.
        iMid = rArray.begin() + nMid - 1;
        ::std::nth_element( rArray.begin(), iMid, rArray.end());
        return (fUp + *iMid) / 2;
    }
}

void ScInterpreter::ScMedian()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 )  )
        return;
    vector<double> aArray;
    GetNumberSequenceArray( nParamCount, aArray);
    PushDouble( GetMedian( aArray));
}

double ScInterpreter::GetPercentile( vector<double> & rArray, double fPercentile )
{
    size_t nSize = rArray.size();
    if (rArray.empty() || nSize == 0 || nGlobalError)
    {
        SetError( errNoValue);
        return 0.0;
    }

    if (nSize == 1)
        return rArray[0];
    else
    {
        size_t nIndex = (size_t)::rtl::math::approxFloor( fPercentile * (nSize-1));
        double fDiff = fPercentile * (nSize-1) - ::rtl::math::approxFloor( fPercentile * (nSize-1));
        DBG_ASSERT(nIndex < nSize, "GetPercentile: wrong index(1)");
        vector<double>::iterator iter = rArray.begin() + nIndex;
        ::std::nth_element( rArray.begin(), iter, rArray.end());
        if (fDiff == 0.0)
            return *iter;
        else
        {
            DBG_ASSERT(nIndex < nSize-1, "GetPercentile: wrong index(2)");
            double fVal = *iter;
            iter = rArray.begin() + nIndex+1;
            ::std::nth_element( rArray.begin(), iter, rArray.end());
            return fVal + fDiff * (*iter - fVal);
        }
    }
}

void ScInterpreter::ScPercentile()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double alpha = GetDouble();
    if (alpha < 0.0 || alpha > 1.0)
    {
        PushIllegalArgument();
        return;
    }
    vector<double> aArray;
    GetNumberSequenceArray( 1, aArray);
    PushDouble( GetPercentile( aArray, alpha));
}

void ScInterpreter::ScQuartile()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fFlag = ::rtl::math::approxFloor(GetDouble());
    if (fFlag < 0.0 || fFlag > 4.0)
    {
        PushIllegalArgument();
        return;
    }
    vector<double> aArray;
    GetNumberSequenceArray( 1, aArray);
    PushDouble( fFlag == 2.0 ? GetMedian( aArray) : GetPercentile( aArray, 0.25 * fFlag));
}

void ScInterpreter::ScModalValue()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    vector<double> aSortArray;
    GetSortArray(nParamCount, aSortArray);
    SCSIZE nSize = aSortArray.size();
    if (aSortArray.empty() || nSize == 0 || nGlobalError)
        PushNoValue();
    else
    {
        SCSIZE nMaxIndex = 0, nMax = 1, nCount = 1;
        double nOldVal = aSortArray[0];
        SCSIZE i;

        for ( i = 1; i < nSize; i++)
        {
            if (aSortArray[i] == nOldVal)
                nCount++;
            else
            {
                nOldVal = aSortArray[i];
                if (nCount > nMax)
                {
                    nMax = nCount;
                    nMaxIndex = i-1;
                }
                nCount = 1;
            }
        }
        if (nCount > nMax)
        {
            nMax = nCount;
            nMaxIndex = i-1;
        }
        if (nMax == 1 && nCount == 1)
            PushNoValue();
        else if (nMax == 1)
            PushDouble(nOldVal);
        else
            PushDouble(aSortArray[nMaxIndex]);
    }
}

void ScInterpreter::ScLarge()
{
    if ( !MustHaveParamCount( GetByte(), 2 )  )
        return;
    double f = ::rtl::math::approxFloor(GetDouble());
    if (f < 1.0)
    {
        PushIllegalArgument();
        return;
    }
    SCSIZE k = static_cast<SCSIZE>(f);
    vector<double> aSortArray;
    /* TODO: using nth_element() is best for one single value, but LARGE/SMALL
     * actually are defined to return an array of values if an array of
     * positions was passed, in which case, depending on the number of values,
     * we may or will need a real sorted array again, see #i32345. */
    //GetSortArray(1, aSortArray);
    GetNumberSequenceArray(1, aSortArray);
    SCSIZE nSize = aSortArray.size();
    if (aSortArray.empty() || nSize == 0 || nGlobalError || nSize < k)
        PushNoValue();
    else
    {
#if 0
/*
        SCSIZE nCount = 1;
        double nOldVal = pSortArray[nSize-1];
        for (long i = nSize-2; i >= 0 && nCount < k; i--)
        {
            if (pSortArray[i] != nOldVal)
            {
                nCount++;
                nOldVal = pSortArray[i];
            }
        }
        if (nCount < k)
            PushNoValue();
        else
            PushDouble(nOldVal);
*/
#endif
        // TODO: the sorted case for array: PushDouble( aSortArray[ nSize-k ] );
        vector<double>::iterator iPos = aSortArray.begin() + nSize-k;
        ::std::nth_element( aSortArray.begin(), iPos, aSortArray.end());
        PushDouble( *iPos);
    }
}

void ScInterpreter::ScSmall()
{
    if ( !MustHaveParamCount( GetByte(), 2 )  )
        return;
    double f = ::rtl::math::approxFloor(GetDouble());
    if (f < 1.0)
    {
        PushIllegalArgument();
        return;
    }
    SCSIZE k = static_cast<SCSIZE>(f);
    vector<double> aSortArray;
    GetSortArray(1, aSortArray);
    SCSIZE nSize = aSortArray.size();
    if (aSortArray.empty() || nSize == 0 || nGlobalError || nSize < k)
        PushNoValue();
    else
    {
#if 0
/*
        SCSIZE nCount = 1;
        double nOldVal = pSortArray[0];
        for (SCSIZE i = 1; i < nSize && nCount < k; i++)
        {
            if (pSortArray[i] != nOldVal)
            {
                nCount++;
                nOldVal = pSortArray[i];
            }
        }
        if (nCount < k)
            PushNoValue();
        else
            PushDouble(nOldVal);
*/
#endif
    PushDouble( aSortArray[ k-1 ] );
    }
}

void ScInterpreter::ScPercentrank()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2 ) )
        return;
#if 0
/*                          wird nicht unterstuetzt
    double fPrec;
    if (nParamCount == 3)
    {
        fPrec = ::rtl::math::approxFloor(GetDouble());
        if (fPrec < 1.0)
        {
            PushIllegalArgument();
            return;
        }
    }
    else
        fPrec = 3.0;
*/
#endif
    double fNum = GetDouble();
    vector<double> aSortArray;
    GetSortArray(1, aSortArray);
    SCSIZE nSize = aSortArray.size();
    if (aSortArray.empty() || nSize == 0 || nGlobalError)
        PushNoValue();
    else
    {
        if (fNum < aSortArray[0] || fNum > aSortArray[nSize-1])
            PushNoValue();
        else if ( nSize == 1 )
            PushDouble(1.0);            // fNum == pSortArray[0], see test above
        else
        {
            double fRes;
            SCSIZE nOldCount = 0;
            double fOldVal = aSortArray[0];
            SCSIZE i;
            for (i = 1; i < nSize && aSortArray[i] < fNum; i++)
            {
                if (aSortArray[i] != fOldVal)
                {
                    nOldCount = i;
                    fOldVal = aSortArray[i];
                }
            }
            if (aSortArray[i] != fOldVal)
                nOldCount = i;
            if (fNum == aSortArray[i])
                fRes = (double)nOldCount/(double)(nSize-1);
            else
            {
                //  #75312# nOldCount is the count of smaller entries
                //  fNum is between pSortArray[nOldCount-1] and pSortArray[nOldCount]
                //  use linear interpolation to find a position between the entries

                if ( nOldCount == 0 )
                {
                    DBG_ERROR("should not happen");
                    fRes = 0.0;
                }
                else
                {
                    double fFract = ( fNum - aSortArray[nOldCount-1] ) /
                        ( aSortArray[nOldCount] - aSortArray[nOldCount-1] );
                    fRes = ( (double)(nOldCount-1)+fFract )/(double)(nSize-1);
                }
            }
            PushDouble(fRes);
        }
    }
}

void ScInterpreter::ScTrimMean()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double alpha = GetDouble();
    if (alpha < 0.0 || alpha >= 1.0)
    {
        PushIllegalArgument();
        return;
    }
    vector<double> aSortArray;
    GetSortArray(1, aSortArray);
    SCSIZE nSize = aSortArray.size();
    if (aSortArray.empty() || nSize == 0 || nGlobalError)
        PushNoValue();
    else
    {
        ULONG nIndex = (ULONG) ::rtl::math::approxFloor(alpha*(double)nSize);
        if (nIndex % 2 != 0)
            nIndex--;
        nIndex /= 2;
        DBG_ASSERT(nIndex < nSize, "ScTrimMean: falscher Index");
        double fSum = 0.0;
        for (SCSIZE i = nIndex; i < nSize-nIndex; i++)
            fSum += aSortArray[i];
        PushDouble(fSum/(double)(nSize-2*nIndex));
    }
}

void ScInterpreter::GetNumberSequenceArray( BYTE nParamCount, vector<double>& rArray )
{
    ScAddress aAdr;
    ScRange aRange;
    short nParam = nParamCount;
    size_t nRefInList = 0;
    while (nParam-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
                rArray.push_back( PopDouble());
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                     rArray.push_back( GetCellValue( aAdr, pCell));
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                PopDoubleRef( aRange, nParam, nRefInList);
                if (nGlobalError)
                    break;

                aRange.Justify();
                SCSIZE nCellCount = aRange.aEnd.Col() - aRange.aStart.Col() + 1;
                nCellCount *= aRange.aEnd.Row() - aRange.aStart.Row() + 1;
                rArray.reserve( rArray.size() + nCellCount);

                USHORT nErr = 0;
                double fCellVal;
                ScValueIterator aValIter(pDok, aRange);
                if (aValIter.GetFirst( fCellVal, nErr))
                {
                    rArray.push_back( fCellVal);
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext( fCellVal, nErr))
                        rArray.push_back( fCellVal);
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (!pMat)
                    break;

                SCSIZE nCount = pMat->GetElementCount();
                rArray.reserve( rArray.size() + nCount);
                if (pMat->IsNumeric())
                {
                    for (SCSIZE i = 0; i < nCount; ++i)
                        rArray.push_back( pMat->GetDouble(i));
                }
                else
                {
                    for (SCSIZE i = 0; i < nCount; ++i)
                        if (!pMat->IsString(i))
                            rArray.push_back( pMat->GetDouble(i));
                }
            }
            break;
            default :
                PopError();
                SetError( errIllegalParameter);
            break;
        }
        if (nGlobalError)
            break;  // while
    }
    // nParam > 0 in case of error, clean stack environment and obtain earlier
    // error if there was one.
    while (nParam-- > 0)
        PopError();
}

void ScInterpreter::GetSortArray( BYTE nParamCount, vector<double>& rSortArray, vector<long>* pIndexOrder )
{
    GetNumberSequenceArray( nParamCount, rSortArray);

    if (rSortArray.size() > MAX_ANZ_DOUBLE_FOR_SORT)
        SetError( errStackOverflow);
    else if (rSortArray.empty())
        SetError( errNoValue);

    if (nGlobalError == 0)
        QuickSort( rSortArray, pIndexOrder);
}

static void lcl_QuickSort( long nLo, long nHi, vector<double>& rSortArray, vector<long>* pIndexOrder )
{
    // If pIndexOrder is not NULL, we assume rSortArray.size() == pIndexOrder->size().

    using ::std::swap;

    if (nHi - nLo == 1)
    {
        if (rSortArray[nLo] > rSortArray[nHi])
        {
            swap(rSortArray[nLo],  rSortArray[nHi]);
            if (pIndexOrder)
                swap(pIndexOrder->at(nLo), pIndexOrder->at(nHi));
        }
        return;
    }

    long ni = nLo;
    long nj = nHi;
    do
    {
        double fLo = rSortArray[nLo];
        while (ni <= nHi && rSortArray[ni] < fLo) ni++;
        while (nj >= nLo && fLo < rSortArray[nj]) nj--;
        if (ni <= nj)
        {
            if (ni != nj)
            {
                swap(rSortArray[ni],  rSortArray[nj]);
                if (pIndexOrder)
                    swap(pIndexOrder->at(ni), pIndexOrder->at(nj));
            }

            ++ni;
            --nj;
        }
    }
    while (ni < nj);

    if ((nj - nLo) < (nHi - ni))
    {
        if (nLo < nj) lcl_QuickSort(nLo, nj, rSortArray, pIndexOrder);
        if (ni < nHi) lcl_QuickSort(ni, nHi, rSortArray, pIndexOrder);
    }
    else
    {
        if (ni < nHi) lcl_QuickSort(ni, nHi, rSortArray, pIndexOrder);
        if (nLo < nj) lcl_QuickSort(nLo, nj, rSortArray, pIndexOrder);
    }
}

void ScInterpreter::QuickSort( vector<double>& rSortArray, vector<long>* pIndexOrder )
{
    long n = static_cast<long>(rSortArray.size());

    if (pIndexOrder)
    {
        pIndexOrder->clear();
        pIndexOrder->reserve(n);
        for (long i = 0; i < n; ++i)
            pIndexOrder->push_back(i);
    }

    if (n < 2)
        return;

    size_t nValCount = rSortArray.size();
    for (size_t i = 0; (i + 4) <= nValCount-1; i += 4)
    {
        size_t nInd = rand() % (int) (nValCount-1);
        ::std::swap( rSortArray[i], rSortArray[nInd]);
        if (pIndexOrder)
            ::std::swap( pIndexOrder->at(i), pIndexOrder->at(nInd));
    }

    lcl_QuickSort(0, n-1, rSortArray, pIndexOrder);
}

void ScInterpreter::ScRank()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    BOOL bDescending;
    if (nParamCount == 3)
        bDescending = GetBool();
    else
        bDescending = FALSE;
    double fCount = 1.0;
    BOOL bValid = FALSE;
    switch (GetStackType())
    {
        case svDouble    :
        {
            double x = GetDouble();
            double fVal = GetDouble();
            if (x == fVal)
                bValid = TRUE;
            break;
        }
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            double fVal = GetDouble();
            ScBaseCell* pCell = GetCell( aAdr );
            if (HasCellValueData(pCell))
            {
                double x = GetCellValue( aAdr, pCell );
                if (x == fVal)
                    bValid = TRUE;
            }
            break;
        }
        case svDoubleRef :
        case svRefList :
        {
            ScRange aRange;
            short nParam = 1;
            size_t nRefInList = 0;
            while (nParam-- > 0)
            {
                USHORT nErr = 0;
                // Preserve stack until all RefList elements are done!
                USHORT nSaveSP = sp;
                PopDoubleRef( aRange, nParam, nRefInList);
                if (nParam)
                    --sp;   // simulate pop
                double fVal = GetDouble();
                if (nParam)
                    sp = nSaveSP;
                double nCellVal;
                ScValueIterator aValIter(pDok, aRange, glSubTotal);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    if (nCellVal == fVal)
                        bValid = TRUE;
                    else if ((!bDescending && nCellVal > fVal) ||
                            (bDescending && nCellVal < fVal) )
                        fCount++;
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(nCellVal, nErr))
                    {
                        if (nCellVal == fVal)
                            bValid = TRUE;
                        else if ((!bDescending && nCellVal > fVal) ||
                                (bDescending && nCellVal < fVal) )
                            fCount++;
                    }
                }
                SetError(nErr);
            }
        }
        break;
        case svMatrix :
        {
            ScMatrixRef pMat = PopMatrix();
            double fVal = GetDouble();
            if (pMat)
            {
                SCSIZE nCount = pMat->GetElementCount();
                if (pMat->IsNumeric())
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                    {
                        double x = pMat->GetDouble(i);
                        if (x == fVal)
                            bValid = TRUE;
                        else if ((!bDescending && x > fVal) ||
                                    (bDescending && x < fVal) )
                            fCount++;
                    }
                }
                else
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                        if (!pMat->IsString(i))
                        {
                            double x = pMat->GetDouble(i);
                            if (x == fVal)
                                bValid = TRUE;
                            else if ((!bDescending && x > fVal) ||
                                        (bDescending && x < fVal) )
                                fCount++;
                        }
                }
            }
        }
        break;
        default : SetError(errIllegalParameter); break;
    }
    if (bValid)
        PushDouble(fCount);
    else
        PushNoValue();
}

void ScInterpreter::ScAveDev()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    USHORT SaveSP = sp;
    double nMiddle = 0.0;
    double rVal = 0.0;
    double rValCount = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    short nParam = nParamCount;
    size_t nRefInList = 0;
    while (nParam-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
                rVal += GetDouble();
                rValCount++;
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    rVal += GetCellValue( aAdr, pCell );
                    rValCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                double nCellVal;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter(pDok, aRange);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    rVal += nCellVal;
                    rValCount++;
                    SetError(nErr);
                    while ((nErr == 0) && aValIter.GetNext(nCellVal, nErr))
                    {
                        rVal += nCellVal;
                        rValCount++;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            rVal += pMat->GetDouble(nElem);
                            rValCount++;
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsString(nElem))
                            {
                                rVal += pMat->GetDouble(nElem);
                                rValCount++;
                            }
                    }
                }
            }
            break;
            default :
                SetError(errIllegalParameter);
            break;
        }
    }
    if (nGlobalError)
    {
        PushError( nGlobalError);
        return;
    }
    nMiddle = rVal / rValCount;
    sp = SaveSP;
    rVal = 0.0;
    nParam = nParamCount;
    nRefInList = 0;
    while (nParam-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
                rVal += fabs(GetDouble() - nMiddle);
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                    rVal += fabs(GetCellValue( aAdr, pCell ) - nMiddle);
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                double nCellVal;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter(pDok, aRange);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    rVal += (fabs(nCellVal - nMiddle));
                    while (aValIter.GetNext(nCellVal, nErr))
                         rVal += fabs(nCellVal - nMiddle);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nCount = pMat->GetElementCount();
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            rVal += fabs(pMat->GetDouble(nElem) - nMiddle);
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            if (!pMat->IsString(nElem))
                                rVal += fabs(pMat->GetDouble(nElem) - nMiddle);
                        }
                    }
                }
            }
            break;
            default : SetError(errIllegalParameter); break;
        }
    }
    PushDouble(rVal / rValCount);
}

void ScInterpreter::ScDevSq()
{
    double nVal;
    double nValCount;
    GetStVarParams(nVal, nValCount);
    PushDouble(nVal);
}

void ScInterpreter::ScProbability()
{
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return;
    double fUp, fLo;
    fUp = GetDouble();
    if (nParamCount == 4)
        fLo = GetDouble();
    else
        fLo = fUp;
    if (fLo > fUp)
    {
        double fTemp = fLo;
        fLo = fUp;
        fUp = fTemp;
    }
    ScMatrixRef pMatP = GetMatrix();
    ScMatrixRef pMatW = GetMatrix();
    if (!pMatP || !pMatW)
        PushIllegalParameter();
    else
    {
        SCSIZE nC1, nC2;
        SCSIZE nR1, nR2;
        pMatP->GetDimensions(nC1, nR1);
        pMatW->GetDimensions(nC2, nR2);
        if (nC1 != nC2 || nR1 != nR2 || nC1 == 0 || nR1 == 0 ||
            nC2 == 0 || nR2 == 0)
            PushNA();
        else
        {
            double fSum = 0.0;
            double fRes = 0.0;
            BOOL bStop = FALSE;
            double fP, fW;
            SCSIZE nCount1 = nC1 * nR1;
            for ( SCSIZE i = 0; i < nCount1 && !bStop; i++ )
            {
                if (pMatP->IsValue(i) && pMatW->IsValue(i))
                {
                    fP = pMatP->GetDouble(i);
                    fW = pMatW->GetDouble(i);
                    if (fP < 0.0 || fP > 1.0)
                        bStop = TRUE;
                    else
                    {
                        fSum += fP;
                        if (fW >= fLo && fW <= fUp)
                            fRes += fP;
                    }
                }
                else
                    SetError( errIllegalArgument);
            }
            if (bStop || fabs(fSum -1.0) > 1.0E-7)
                PushNoValue();
            else
                PushDouble(fRes);
        }
    }
}

void ScInterpreter::ScCorrel()
{
    // This is identical to ScPearson() 
    ScPearson();
}

void ScInterpreter::ScCovar()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    /* #i78250#
     * (sum((X-MeanX)(Y-MeanY)))/N equals (SumXY)/N-MeanX*MeanY mathematically,
     * but the latter produces wrong results if the absolute values are high,
     * for example above 10^8
     */
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                }
            }
        }
        PushDouble( fSumDeltaXDeltaY / fCount);
    }
}

void ScInterpreter::ScPearson() 
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    /* #i78250#
     * (sum((X-MeanX)(Y-MeanY)))/N equals (SumXY)/N-MeanX*MeanY mathematically,
     * but the latter produces wrong results if the absolute values are high,
     * for example above 10^8
     */
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    double fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
    double fSumSqrDeltaY    = 0.0; // sum of (ValY-MeanY)^2
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 1.0) // fCount==1 is handled by checking denominator later on
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                    fSumSqrDeltaY    += (fValY - fMeanY) * (fValY - fMeanY);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0 || fSumSqrDeltaY == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble( fSumDeltaXDeltaY / sqrt( fSumSqrDeltaX * fSumSqrDeltaY));
    }
}

void ScInterpreter::ScRSQ()
{
    // Same as ScPearson()*ScPearson()
    ScPearson();
    if (!nGlobalError)
    {
        switch (GetStackType())
        {
            case svDouble:
                {
                    double fVal = PopDouble();
                    PushDouble( fVal * fVal);
                }
                break;
            default:
                PopError();
                PushNoValue();
        }
    }
}

void ScInterpreter::ScSTEXY()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    // #i78250# numerical stability improved
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    double fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
    double fSumSqrDeltaY    = 0.0; // sum of (ValY-MeanY)^2
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 3.0)
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                    fSumSqrDeltaY    += (fValY - fMeanY) * (fValY - fMeanY);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble( sqrt( (fSumSqrDeltaY - fSumDeltaXDeltaY *
                            fSumDeltaXDeltaY / fSumSqrDeltaX) / (fCount-2)));
    }
}

void ScInterpreter::ScSlope()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    // #i78250# numerical stability improved
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    double fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble( fSumDeltaXDeltaY / fSumSqrDeltaX);
    }
}

void ScInterpreter::ScIntercept()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    // #i78250# numerical stability improved
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    double fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble( fMeanY - fSumDeltaXDeltaY / fSumSqrDeltaX * fMeanX);
    }
}

void ScInterpreter::ScForecast()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    double fVal = GetDouble();
    // #i78250# numerical stability improved
    double fCount           = 0.0;
    double fSumX            = 0.0;
    double fSumY            = 0.0;
    double fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
    double fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
            {
                double fValX = pMat1->GetDouble(i,j);
                double fValY = pMat2->GetDouble(i,j);
                fSumX += fValX;
                fSumY += fValY;
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
        PushNoValue();
    else
    {
        double fMeanX = fSumX / fCount;
        double fMeanY = fSumY / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsString(i,j) && !pMat2->IsString(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0)
            PushError( errDivisionByZero);
        else
            PushDouble( fMeanY + fSumDeltaXDeltaY / fSumSqrDeltaX * (fVal - fMeanX));
    }
}

