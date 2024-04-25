/* e_j0f.c -- float version of e_j0.c.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <math.h>
#include <math-barriers.h>
#include <math_private.h>
#include <fenv_private.h>
#include <libm-alias-finite.h>
#include <reduce_aux.h>

static float pzerof(float), qzerof(float);

static const float
huge	= 1e30,
one	= 1.0,
invsqrtpi=  5.6418961287e-01, /* 0x3f106ebb */
tpi      =  6.3661974669e-01, /* 0x3f22f983 */
		/* R0/S0 on [0, 2.00] */
R02  =  1.5625000000e-02, /* 0x3c800000 */
R03  = -1.8997929874e-04, /* 0xb947352e */
R04  =  1.8295404516e-06, /* 0x35f58e88 */
R05  = -4.6183270541e-09, /* 0xb19eaf3c */
S01  =  1.5619102865e-02, /* 0x3c7fe744 */
S02  =  1.1692678527e-04, /* 0x38f53697 */
S03  =  5.1354652442e-07, /* 0x3509daa6 */
S04  =  1.1661400734e-09; /* 0x30a045e8 */

static const float zero = 0.0;

/* This is the nearest approximation of the first zero of j0.  */
#define FIRST_ZERO_J0 0x1.33d152p+1f

#define SMALL_SIZE 64

/* The following table contains successive zeros of j0 and degree-3
   polynomial approximations of j0 around these zeros: Pj[0] for the first
   zero (2.40482), Pj[1] for the second one (5.520078), and so on.
   Each line contains:
              {x0, xmid, x1, p0, p1, p2, p3}
   where [x0,x1] is the interval around the zero, xmid is the binary32 number
   closest to the zero, and p0+p1*x+p2*x^2+p3*x^3 is the approximation
   polynomial.  Each polynomial was generated using Sollya on the interval
   [x0,x1] around the corresponding zero where the error exceeds 9 ulps
   for the alternate code.  Degree 3 is enough to get an error <= 9 ulps.
*/
static const float Pj[SMALL_SIZE][7] = {
  /* The following polynomial was optimized by hand with respect to the one
     generated by Sollya, to ensure the maximal error is at most 9 ulps,
     both if the polynomial is evaluated with fma or not.  */
  { 0x1.31e5c4p+1, 0x1.33d152p+1, 0x1.3b58dep+1, 0xf.2623fp-28,
    -0x8.4e6d7p-4, 0x1.ba2aaap-4, 0xe.4b9ap-8 }, /* 0 */
  { 0x1.60eafap+2, 0x1.6148f6p+2, 0x1.62955cp+2, 0x6.9205fp-28,
    0x5.71b98p-4, -0x7.e3e798p-8, -0xd.87d1p-8 }, /* 1 */
  { 0x1.14cde2p+3, 0x1.14eb56p+3, 0x1.1525c6p+3, 0x1.bcc1cap-24,
    -0x4.57de6p-4, 0x4.03e7cp-8, 0xb.39a37p-8 }, /* 2 */
  { 0x1.7931d8p+3, 0x1.79544p+3, 0x1.7998d6p+3, -0xf.2976fp-32,
    0x3.b827ccp-4, -0x2.8603ep-8, -0x9.bf49bp-8 }, /* 3 */
  { 0x1.ddb6d4p+3, 0x1.ddca14p+3, 0x1.ddf0c8p+3, -0x1.bd67d8p-28,
    -0x3.4e03ap-4, 0x1.c562a2p-8, 0x8.90ec2p-8 }, /* 4 */
  { 0x1.2118e4p+4, 0x1.212314p+4, 0x1.21375p+4, 0x1.62209cp-28,
    0x3.00efecp-4, -0x1.5458dap-8, -0x8.10063p-8 }, /* 5 */
  { 0x1.535d28p+4, 0x1.5362dep+4, 0x1.536e48p+4, -0x2.853f74p-24,
    -0x2.c5b274p-4, 0x1.0b9db4p-8, 0x7.8c3578p-8 }, /* 6 */
  { 0x1.859ddp+4, 0x1.85a3bap+4, 0x1.85aff4p+4, 0x2.19ed1cp-24,
    0x2.96545cp-4, -0xd.997e6p-12, -0x6.d9af28p-8 }, /* 7 */
  { 0x1.b7decap+4, 0x1.b7e54ap+4, 0x1.b7f038p+4, 0xe.959aep-28,
    -0x2.6f5594p-4, 0xb.538dp-12, 0x7.003ea8p-8 }, /* 8 */
  { 0x1.ea21c6p+4, 0x1.ea275ap+4, 0x1.ea337ap+4, 0x2.0c3964p-24,
    0x2.4e80fcp-4, -0x9.a2216p-12, -0x6.61e0a8p-8 }, /* 9 */
  { 0x1.0e3316p+5, 0x1.0e34e2p+5, 0x1.0e379ap+5, -0x3.642554p-24,
    -0x2.325e48p-4, 0x8.4f49cp-12, 0x7.d37c3p-8 }, /* 10 */
  { 0x1.275456p+5, 0x1.275638p+5, 0x1.2759e2p+5, 0x1.6c015ap-24,
    0x2.19e7d8p-4, -0x7.4c1bf8p-12, -0x4.af7ef8p-8 }, /* 11 */
  { 0x1.4075ecp+5, 0x1.4077a8p+5, 0x1.407b96p+5, -0x4.b18c9p-28,
    -0x2.046174p-4, 0x6.705618p-12, 0x5.f2d28p-8 }, /* 12 */
  { 0x1.59973p+5, 0x1.59992cp+5, 0x1.599b2ap+5, -0x1.8b8792p-24,
    0x1.f13fbp-4, -0x5.c14938p-12, -0x5.73e0cp-8 }, /* 13 */
  { 0x1.72b958p+5, 0x1.72bacp+5, 0x1.72bc5ap+5, 0x3.a26e0cp-24,
    -0x1.e018dap-4, 0x5.30e8dp-12, 0x2.81099p-8 }, /* 14 */
  { 0x1.8bdb4ap+5, 0x1.8bdc62p+5, 0x1.8bde7ep+5, -0x2.18fabcp-24,
    0x1.d09b22p-4, -0x4.b0b688p-12, -0x5.5fd308p-8 }, /* 15 */
  { 0x1.a4fcecp+5, 0x1.a4fe0ep+5, 0x1.a50042p+5, 0x3.2370e8p-24,
    -0x1.c28614p-4, 0x4.4647e8p-12, 0x5.68a28p-8 }, /* 16 */
  { 0x1.be1ebcp+5, 0x1.be1fc4p+5, 0x1.be21fp+5, -0x5.9eae3p-28,
    0x1.b5a622p-4, -0x3.eb9054p-12, -0x5.12d8cp-8 }, /* 17 */
  { 0x1.d7405p+5, 0x1.d7418p+5, 0x1.d74294p+5, 0x2.9fa1e8p-24,
    -0x1.a9d184p-4, 0x3.9d1e7p-12, 0x4.33d058p-8 }, /* 18 */
  { 0x1.f0624p+5, 0x1.f06344p+5, 0x1.f0645ep+5, 0x9.9ac67p-28,
    0x1.9ee5eep-4, -0x3.5816e8p-12, -0x2.6e5004p-8 }, /* 19 */
  { 0x1.04c22ep+6, 0x1.04c286p+6, 0x1.04c316p+6, 0xd.6ab94p-28,
    -0x1.94c6f6p-4, 0x3.174efcp-12, 0x7.9a092p-8 }, /* 20 */
  { 0x1.1153p+6, 0x1.11536cp+6, 0x1.11541p+6, -0x4.4cb2d8p-24,
    0x1.8b5cccp-4, -0x2.e3c238p-12, -0x4.e5437p-8 }, /* 21 */
  { 0x1.1de3d8p+6, 0x1.1de456p+6, 0x1.1de4dap+6, -0x4.4aa8c8p-24,
    -0x1.829356p-4, 0x2.b45124p-12, 0x5.baf638p-8 }, /* 22 */
  { 0x1.2a74f8p+6, 0x1.2a754p+6, 0x1.2a75bp+6, 0x2.077c38p-24,
    0x1.7a597ep-4, -0x2.8a0414p-12, -0x2.838d3p-8 }, /* 23 */
  { 0x1.3705d4p+6, 0x1.37062cp+6, 0x1.3706b2p+6, -0x2.6a6cd8p-24,
    -0x1.72a09ap-4, 0x2.623a3cp-12, 0x5.5256a8p-8 }, /* 24 */
  { 0x1.4396dp+6, 0x1.439718p+6, 0x1.43976ep+6, -0x5.08287p-24,
    0x1.6b5c06p-4, -0x2.3da154p-12, -0x7.a2254p-8 }, /* 25 */
  { 0x1.5027acp+6, 0x1.502808p+6, 0x1.50288cp+6, -0x3.4598dcp-24,
    -0x1.6480c4p-4, 0x2.1cb944p-12, 0x7.27c77p-8 }, /* 26 */
  { 0x1.5cb89ap+6, 0x1.5cb8f8p+6, 0x1.5cb97ep+6, 0x5.4e74bp-24,
    0x1.5e0544p-4, -0x2.00b158p-12, -0x5.9bc4a8p-8 }, /* 27 */
  { 0x1.69498cp+6, 0x1.6949e8p+6, 0x1.694a42p+6, -0x2.05751cp-24,
    -0x1.57e12p-4, 0x1.e78edcp-12, 0x9.9667dp-8 }, /* 28 */
  { 0x1.75da7ep+6, 0x1.75dadap+6, 0x1.75db3p+6, 0x4.c5e278p-24,
    0x1.520ceep-4, -0x1.d0127cp-12, -0xd.62681p-8 }, /* 29 */
  { 0x1.826b7ep+6, 0x1.826bccp+6, 0x1.826c2cp+6, -0x3.50e62cp-24,
    -0x1.4c822p-4, 0x1.ba5832p-12, -0x1.eb2ee2p-8 }, /* 30 */
  { 0x1.8efc84p+6, 0x1.8efcbep+6, 0x1.8efd16p+6, -0x1.c39f38p-24,
    0x1.473ae6p-4, -0x1.a616c8p-12, 0xf.f352ap-12 }, /* 31 */
  { 0x1.9b8d84p+6, 0x1.9b8db2p+6, 0x1.9b8e7p+6, -0x1.9245b6p-28,
    -0x1.42320ap-4, 0x1.932a04p-12, 0x2.dc113cp-8 }, /* 32 */
  { 0x1.a81e72p+6, 0x1.a81ea6p+6, 0x1.a81f04p+6, -0x1.0acf8p-24,
    0x1.3d62e6p-4, -0x1.7c4b14p-12, -0x1.cfc5c2p-4 }, /* 33 */
  { 0x1.b4af6ap+6, 0x1.b4af9ap+6, 0x1.b4afeep+6, 0x4.cd92d8p-24,
    -0x1.38c94ap-4, 0x1.643154p-12, 0x1.4c2a06p-4 }, /* 34 */
  { 0x1.c1406p+6, 0x1.c1409p+6, 0x1.c140cp+6, -0x1.37bf8ap-24,
    0x1.34617p-4, -0x1.5f504ap-12, -0x1.e2d324p-4 }, /* 35 */
  { 0x1.cdd154p+6, 0x1.cdd186p+6, 0x1.cdd1eap+6, -0x1.8f62dep-28,
    -0x1.3027fp-4, 0x1.534a02p-12, 0x2.c7f144p-12 }, /* 36 */
  { 0x1.da6248p+6, 0x1.da627cp+6, 0x1.da62e6p+6, -0x9.81e79p-28,
    0x1.2c19b4p-4, -0x1.4b8288p-12, 0x7.2d8bap-8 }, /* 37 */
  { 0x1.e6f33ep+6, 0x1.e6f372p+6, 0x1.e6f3a8p+6, 0x3.103b3p-24,
    -0x1.2833eep-4, 0x1.36f4d2p-12, 0x9.29f91p-8 }, /* 38 */
  { 0x1.f38434p+6, 0x1.f3846ap+6, 0x1.f384d8p+6, 0x2.07b058p-24,
    0x1.24740ap-4, -0x1.2ee58ap-12, 0xd.f1393p-12 }, /* 39 */
  { 0x1.000a98p+7, 0x1.000abp+7, 0x1.000ac8p+7, 0x3.87576cp-24,
    -0x1.20d7b6p-4, 0x1.2083e2p-12, 0x3.9a7aap-8 }, /* 40 */
  { 0x1.06531p+7, 0x1.06532cp+7, 0x1.065348p+7, -0x1.691ecp-24,
    0x1.1d5ccap-4, -0x1.166726p-12, -0x1.e4af48p-8 }, /* 41 */
  { 0x1.0c9b9ap+7, 0x1.0c9ba8p+7, 0x1.0c9bbep+7, 0x9.b406dp-28,
    -0x1.1a015p-4, 0x1.038f9cp-12, -0x4.021058p-4 }, /* 42 */
  { 0x1.12e412p+7, 0x1.12e424p+7, 0x1.12e436p+7, -0xf.bfd8fp-28,
    0x1.16c37ap-4, -0x1.039edep-12, 0x1.f0033p-4 }, /* 43 */
  { 0x1.192c92p+7, 0x1.192cap+7, 0x1.192cb6p+7, 0x2.6d50c8p-24,
    -0x1.13a19ep-4, 0xf.9df8ap-16, 0x4.ecd978p-8 }, /* 44 */
  { 0x1.1f7512p+7, 0x1.1f751cp+7, 0x1.1f753ap+7, -0x4.d475c8p-24,
    0x1.109a32p-4, -0x1.04fb3ap-12, -0xd.c271p-12 }, /* 45 */
  { 0x1.25bd8ep+7, 0x1.25bd98p+7, 0x1.25bdap+7, 0x8.1982p-24,
    -0x1.0dabc8p-4, 0xe.88eabp-16, -0x4.ed75dp-4 }, /* 46 */
  { 0x1.2c060cp+7, 0x1.2c0616p+7, 0x1.2c0644p+7, 0x4.864518p-24,
    0x1.0ad51p-4, -0xe.27196p-16, 0xb.97a3ep-8 }, /* 47 */
  { 0x1.324e86p+7, 0x1.324e92p+7, 0x1.324e9ep+7, 0x6.8917a8p-28,
    -0x1.0814d4p-4, 0xd.4fe7ep-16, -0x6.8d8d6p-4 }, /* 48 */
  { 0x1.389702p+7, 0x1.38970ep+7, 0x1.389728p+7, -0x5.fa18fp-24,
    0x1.0569fp-4, -0xd.5b0d4p-16, 0x1.50353ap-4 }, /* 49 */
  { 0x1.3edf84p+7, 0x1.3edf8cp+7, 0x1.3edfaap+7, -0x4.0e5c98p-24,
    -0x1.02d354p-4, 0xb.7b255p-16, 0x7.8a916p-4 }, /* 50 */
  { 0x1.4527fp+7, 0x1.452808p+7, 0x1.452812p+7, -0x2.c3ddbp-24,
    0x1.005004p-4, -0xd.7729cp-16, -0x3.bcc354p-8 }, /* 51 */
  { 0x1.4b7076p+7, 0x1.4b7086p+7, 0x1.4b70a4p+7, -0x5.d052p-24,
    -0xf.ddf16p-8, 0xc.318c1p-16, 0x5.7947p-8 }, /* 52 */
  { 0x1.51b8f4p+7, 0x1.51b902p+7, 0x1.51b90ep+7, -0x2.0b97dcp-24,
    0xf.b7fafp-8, -0xc.1429dp-16, -0x3.43c36p-4 }, /* 53 */
  { 0x1.580168p+7, 0x1.58018p+7, 0x1.580188p+7, -0x5.4aab5p-24,
    -0xf.930fep-8, 0xa.ecc24p-16, 0x9.c62cdp-12 }, /* 54 */
  { 0x1.5e49eap+7, 0x1.5e49fcp+7, 0x1.5e4a12p+7, -0x3.6dadd8p-24,
    0xf.6f245p-8, -0xb.6816cp-16, 0xa.d731ap-8 }, /* 55 */
  { 0x1.649272p+7, 0x1.64927ap+7, 0x1.64929p+7, -0x2.d7e038p-24,
    -0xf.4c2cep-8, 0xb.118bep-16, 0xb.69a4ep-8 }, /* 56 */
  { 0x1.6adae6p+7, 0x1.6adaf6p+7, 0x1.6adb04p+7, -0x6.977a1p-24,
    0xf.2a1fp-8, -0xa.a8911p-16, -0x4.bf6d2p-8 }, /* 57 */
  { 0x1.712366p+7, 0x1.712374p+7, 0x1.71238ep+7, 0x1.3cc95ep-24,
    -0xf.08f0ap-8, 0x9.f0858p-16, 0x1.77f7f4p-4 }, /* 58 */
  { 0x1.776beap+7, 0x1.776bf2p+7, 0x1.776bfap+7, 0x3.a4921p-24,
    0xe.e8986p-8, -0xa.39dfp-16, -0x6.7ba3dp-4 }, /* 59 */
  { 0x1.7db464p+7, 0x1.7db46ep+7, 0x1.7db476p+7, 0x6.b45a7p-24,
    -0xe.c90d8p-8, 0xa.e586fp-16, -0x1.d66becp-4 }, /* 60 */
  { 0x1.83fce2p+7, 0x1.83fcecp+7, 0x1.83fd0ep+7, -0x2.8f34a4p-24,
    0xe.aa478p-8, -0x9.810bp-16, -0x3.a5f3fcp-8 }, /* 61 */
  { 0x1.8a455cp+7, 0x1.8a456ap+7, 0x1.8a4588p+7, -0x1.325968p-24,
    -0xe.8c3eap-8, 0x9.0a765p-16, 0x1.29a54ap-4 }, /* 62 */
  { 0x1.908dd8p+7, 0x1.908de8p+7, 0x1.908df4p+7, 0x4.96b808p-24,
    0xe.6eeb5p-8, -0x9.0251bp-16, 0x1.41a488p-4 }, /* 63 */
};

/* Formula page 5 of https://www.cl.cam.ac.uk/~jrh13/papers/bessel.pdf:
   j0f(x) ~ sqrt(2/(pi*x))*beta0(x)*cos(x-pi/4-alpha0(x))
   where beta0(x) = 1 - 1/(16*x^2) + 53/(512*x^4)
   and alpha0(x) = 1/(8*x) - 25/(384*x^3).  */
static float
j0f_asympt (float x)
{
  /* The following code fails to give an error <= 9 ulps in only two cases,
     for which we tabulate the result.  */
  if (x == 0x1.4665d2p+24f)
    return 0xa.50206p-52f;
  if (x == 0x1.a9afdep+7f)
    return 0xf.47039p-28f;
  double y = 1.0 / (double) x;
  double y2 = y * y;
  double beta0 = 1.0f + y2 * (-0x1p-4f + 0x1.a8p-4 * y2);
  double alpha0 = y * (0x2p-4 - 0x1.0aaaaap-4 * y2);
  double h;
  int n;
  h = reduce_aux (x, &n, alpha0);
  /* Now x - pi/4 - alpha0 = h + n*pi/2 mod (2*pi).  */
  float xr = (float) h;
  n = n & 3;
  float cst = 0xc.c422ap-4f; /* sqrt(2/pi) rounded to nearest  */
  float t = cst / sqrtf (x) * (float) beta0;
  if (n == 0)
    return t * __cosf (xr);
  else if (n == 2) /* cos(x+pi) = -cos(x)  */
    return -t * __cosf (xr);
  else if (n == 1) /* cos(x+pi/2) = -sin(x)  */
    return -t * __sinf (xr);
  else             /* cos(x+3pi/2) = sin(x)  */
    return t * __sinf (xr);
}

/* Special code for x near a root of j0.
   z is the value computed by the generic code.
   For small x, we use a polynomial approximating j0 around its root.
   For large x, we use an asymptotic formula (j0f_asympt).  */
static float
j0f_near_root (float x, float z)
{
  float index_f;
  int index;

  index_f = roundf ((x - FIRST_ZERO_J0) / M_PIf);
  /* j0f_asympt fails to give an error <= 9 ulps for x=0x1.324e92p+7
     (index 48) thus we can't reduce SMALL_SIZE below 49.  */
  if (index_f >= SMALL_SIZE)
    return j0f_asympt (x);
  index = (int) index_f;
  const float *p = Pj[index];
  float x0 = p[0];
  float x1 = p[2];
  /* If not in the interval [x0,x1] around xmid, we return the value z.  */
  if (! (x0 <= x && x <= x1))
    return z;
  float xmid = p[1];
  float y = x - xmid;
  return p[3] + y * (p[4] + y * (p[5] + y * p[6]));
}

float
__ieee754_j0f(float x)
{
	float z, s,c,ss,cc,r,u,v;
	int32_t hx,ix;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x7f800000) return one/(x*x);
	x = fabsf(x);
	if(ix >= 0x40000000) {	/* |x| >= 2.0 */
                SET_RESTORE_ROUNDF (FE_TONEAREST);
		__sincosf (x, &s, &c);
		ss = s-c;
		cc = s+c;
                if (ix >= 0x7f000000)
		  /* x >= 2^127: use asymptotic expansion.  */
                  return j0f_asympt (x);
                /* Now we are sure that x+x cannot overflow.  */
                z = -__cosf(x+x);
                if ((s*c)<zero) cc = z/ss;
                else	    ss = z/cc;
	/*
	 * j0(x) = 1/sqrt(pi) * (P(0,x)*cc - Q(0,x)*ss) / sqrt(x)
	 * y0(x) = 1/sqrt(pi) * (P(0,x)*ss + Q(0,x)*cc) / sqrt(x)
	 */
                if (ix <= 0x5c000000)
                  {
                    u = pzerof(x); v = qzerof(x);
                    cc = u*cc-v*ss;
                  }
                z = (invsqrtpi * cc) / sqrtf(x);
                /* The following threshold is optimal: for x=0x1.3b58dep+1
                   and rounding upwards, |cc|=0x1.579b26p-4 and z is 10 ulps
                   far from the correctly rounded value.  */
                float threshold = 0x1.579b26p-4f;
                if (fabsf (cc) > threshold)
                  return z;
                else
                  return j0f_near_root (x, z);
	}
	if(ix<0x39000000) {	/* |x| < 2**-13 */
	    math_force_eval(huge+x);		/* raise inexact if x != 0 */
	    if(ix<0x32000000) return one;	/* |x|<2**-27 */
	    else	      return one - (float)0.25*x*x;
	}
	z = x*x;
	r =  z*(R02+z*(R03+z*(R04+z*R05)));
	s =  one+z*(S01+z*(S02+z*(S03+z*S04)));
	if(ix < 0x3F800000) {	/* |x| < 1.00 */
	    return one + z*((float)-0.25+(r/s));
	} else {
	    u = (float)0.5*x;
	    return((one+u)*(one-u)+z*(r/s));
	}
}
libm_alias_finite (__ieee754_j0f, __j0f)

static const float
u00  = -7.3804296553e-02, /* 0xbd9726b5 */
u01  =  1.7666645348e-01, /* 0x3e34e80d */
u02  = -1.3818567619e-02, /* 0xbc626746 */
u03  =  3.4745343146e-04, /* 0x39b62a69 */
u04  = -3.8140706238e-06, /* 0xb67ff53c */
u05  =  1.9559013964e-08, /* 0x32a802ba */
u06  = -3.9820518410e-11, /* 0xae2f21eb */
v01  =  1.2730483897e-02, /* 0x3c509385 */
v02  =  7.6006865129e-05, /* 0x389f65e0 */
v03  =  2.5915085189e-07, /* 0x348b216c */
v04  =  4.4111031494e-10; /* 0x2ff280c2 */

/* This is the nearest approximation of the first zero of y0.  */
#define FIRST_ZERO_Y0 0xe.4c166p-4f

/* The following table contains successive zeros of y0 and degree-3
   polynomial approximations of y0 around these zeros: Py[0] for the first
   zero (0.89358), Py[1] for the second one (3.957678), and so on.
   Each line contains:
              {x0, xmid, x1, p0, p1, p2, p3}
   where [x0,x1] is the interval around the zero, xmid is the binary32 number
   closest to the zero, and p0+p1*x+p2*x^2+p3*x^3 is the approximation
   polynomial.  Each polynomial was generated using Sollya on the interval
   [x0,x1] around the corresponding zero where the error exceeds 9 ulps
   for the alternate code.  Degree 3 is enough, except for index 0 where we
   use degree 5, and the coefficients of degree 4 and 5 are hard-coded in
   y0f_near_root.
*/
static const float Py[SMALL_SIZE][7] = {
  { 0x1.a681dap-1, 0x1.c982ecp-1, 0x1.ef6bcap-1, 0x3.274468p-28,
    0xe.121b8p-4, -0x7.df8b3p-4, 0x3.877be4p-4
    /*, -0x3.a46c9p-4, 0x3.735478p-4*/ }, /* 0 */
  { 0x1.f957c6p+1, 0x1.fa9534p+1, 0x1.fd11b2p+1, 0xa.f1f83p-28,
    -0x6.70d098p-4, 0xd.04d48p-8, 0xe.f61a9p-8 }, /* 1 */
  { 0x1.c51832p+2, 0x1.c581dcp+2, 0x1.c65164p+2, -0x5.e2a51p-28,
    0x4.cd3328p-4, -0x5.6bbe08p-8, -0xc.46d8p-8 }, /* 2 */
  { 0x1.46fd84p+3, 0x1.471d74p+3, 0x1.475bfcp+3, -0x1.4b0aeep-24,
    -0x3.fec6b8p-4, 0x3.2068a4p-8, 0xa.76e2dp-8 }, /* 3 */
  { 0x1.ab7afap+3, 0x1.ab8e1cp+3, 0x1.abb294p+3, -0x8.179d7p-28,
    0x3.7e6544p-4, -0x2.1799fp-8, -0x9.0e1c4p-8 }, /* 4 */
  { 0x1.07f9aap+4, 0x1.0803c8p+4, 0x1.08170cp+4, -0x2.5b8078p-24,
    -0x3.24b868p-4, 0x1.8631ecp-8, 0x8.3cb46p-8 }, /* 5 */
  { 0x1.3a38eap+4, 0x1.3a42cep+4, 0x1.3a4d8ap+4, 0x1.cd304ap-28,
    0x2.e189ecp-4, -0x1.2c6954p-8, -0x7.8178ep-8 }, /* 6 */
  { 0x1.6c7d42p+4, 0x1.6c833p+4, 0x1.6c99fp+4, -0x6.c63f1p-28,
    -0x2.acc9a8p-4, 0xf.09e31p-12, 0x7.0b5ab8p-8 }, /* 7 */
  { 0x1.9ebec4p+4, 0x1.9ec47p+4, 0x1.9ed016p+4, 0x1.e53838p-24,
    0x2.81f2p-4, -0xc.5ff51p-12, -0x7.05ep-8 }, /* 8 */
  { 0x1.d1008ep+4, 0x1.d10644p+4, 0x1.d11262p+4, 0x1.636feep-24,
    -0x2.5e40dcp-4, 0xa.6f81dp-12, 0x5.ff6p-8 }, /* 9 */
  { 0x1.01a27cp+5, 0x1.01a442p+5, 0x1.01a924p+5, -0x4.04e1bp-28,
    0x2.3febd8p-4, -0x8.f11e2p-12, -0x6.0111ap-8 }, /* 10 */
  { 0x1.1ac3bcp+5, 0x1.1ac588p+5, 0x1.1ac912p+5, 0x3.6063d8p-24,
    -0x2.25baacp-4, 0x7.c93cdp-12, 0x4.e7577p-8 }, /* 11 */
  { 0x1.33e508p+5, 0x1.33e6ecp+5, 0x1.33ea1ap+5, -0x3.f39ebcp-24,
    0x2.0ed04cp-4, -0x6.d9434p-12, -0x4.fc0b7p-8 }, /* 12 */
  { 0x1.4d0666p+5, 0x1.4d0868p+5, 0x1.4d0c14p+5, -0x4.ea23p-28,
    -0x1.fa8b4p-4, 0x6.1470e8p-12, 0x5.97f71p-8 }, /* 13 */
  { 0x1.6628b8p+5, 0x1.6629f4p+5, 0x1.662e0ep+5, -0x3.5df0c8p-24,
    0x1.e8727ep-4, -0x5.76a038p-12, -0x4.ee37c8p-8 }, /* 14 */
  { 0x1.7f4a7cp+5, 0x1.7f4b9p+5, 0x1.7f4daap+5, 0x1.1ef09ep-24,
    -0x1.d8293ap-4, 0x4.ed8a28p-12, 0x4.d43708p-8 }, /* 15 */
  { 0x1.986c5cp+5, 0x1.986d38p+5, 0x1.986f6p+5, 0x1.b70cecp-24,
    0x1.c967p-4, -0x4.7a70b8p-12, -0x5.6840e8p-8 }, /* 16 */
  { 0x1.b18dcap+5, 0x1.b18ee8p+5, 0x1.b19122p+5, 0x1.abaadcp-24,
    -0x1.bbf246p-4, 0x4.1a35bp-12, 0x3.c2d46p-8 }, /* 17 */
  { 0x1.caaf86p+5, 0x1.cab0a2p+5, 0x1.cab2fep+5, 0x1.63989ep-24,
    0x1.af9cb4p-4, -0x3.c2f2dcp-12, -0x4.cf8108p-8 }, /* 18 */
  { 0x1.e3d146p+5, 0x1.e3d262p+5, 0x1.e3d492p+5, -0x1.68a8ecp-24,
    -0x1.a4407ep-4, 0x3.7733ecp-12, 0x5.97916p-8 }, /* 19 */
  { 0x1.fcf316p+5, 0x1.fcf428p+5, 0x1.fcf59ap+5, 0x1.e1bb5p-24,
    0x1.99be74p-4, -0x3.37210cp-12, -0x5.d19bf8p-8 }, /* 20 */
  { 0x1.0b0a7cp+6, 0x1.0b0afap+6, 0x1.0b0b9cp+6, -0x5.5bbcfp-24,
    -0x1.8ffc9ap-4, 0x2.ffe638p-12, 0x2.ed28e8p-8 }, /* 21 */
  { 0x1.179b66p+6, 0x1.179bep+6, 0x1.179d0ap+6, -0x4.9e34a8p-24,
    0x1.86e51cp-4, -0x2.cc7a68p-12, -0x3.3642c4p-8 }, /* 22 */
  { 0x1.242c5cp+6, 0x1.242ccap+6, 0x1.242d68p+6, 0x1.966706p-24,
    -0x1.7e657p-4, 0x2.9aed4cp-12, 0x7.b87a58p-8 }, /* 23 */
  { 0x1.30bd62p+6, 0x1.30bdb6p+6, 0x1.30beb2p+6, 0x3.4b3b68p-24,
    0x1.766dc2p-4, -0x2.72651cp-12, -0x3.e347f8p-8 }, /* 24 */
  { 0x1.3d4e56p+6, 0x1.3d4ea2p+6, 0x1.3d4f2ep+6, 0x6.a99008p-28,
    -0x1.6ef07ep-4, 0x2.53aec4p-12, 0x2.9e3d88p-12 }, /* 25 */
  { 0x1.49df38p+6, 0x1.49df9p+6, 0x1.49e042p+6, -0x7.a9fa6p-32,
    0x1.67e1dap-4, -0x2.324d7p-12, -0xc.0e669p-12 }, /* 26 */
  { 0x1.56702ep+6, 0x1.56708p+6, 0x1.567116p+6, -0x5.026808p-24,
    -0x1.613798p-4, 0x2.114594p-12, 0x1.a22402p-8 }, /* 27 */
  { 0x1.630126p+6, 0x1.63017p+6, 0x1.630226p+6, 0x4.46aa2p-24,
    0x1.5ae8c2p-4, -0x1.f4aaa4p-12, -0x3.58593cp-8 }, /* 28 */
  { 0x1.6f9234p+6, 0x1.6f926p+6, 0x1.6f92b2p+6, 0x1.5cfccp-24,
    -0x1.54ed76p-4, 0x1.dd540ap-12, -0xb.e9429p-12 }, /* 29 */
  { 0x1.7c22fep+6, 0x1.7c2352p+6, 0x1.7c23c2p+6, -0xb.4dc4cp-28,
    0x1.4f3ebcp-4, -0x1.c463fp-12, -0x1.e94c54p-8 }, /* 30 */
  { 0x1.88b412p+6, 0x1.88b444p+6, 0x1.88b50ap+6, 0x3.f5343p-24,
    -0x1.49d668p-4, 0x1.a53f24p-12, 0x5.128198p-8 }, /* 31 */
  { 0x1.9544dcp+6, 0x1.954538p+6, 0x1.95459p+6, -0x6.e6f32p-28,
    0x1.44aefap-4, -0x1.9a6ef8p-12, -0x6.c639cp-8 }, /* 32 */
  { 0x1.a1d5fap+6, 0x1.a1d62cp+6, 0x1.a1d674p+6, 0x1.f359c2p-28,
    -0x1.3fc386p-4, 0x1.887ebep-12, 0x1.6c813cp-8 }, /* 33 */
  { 0x1.ae66cp+6, 0x1.ae672p+6, 0x1.ae6788p+6, -0x2.9de748p-24,
    0x1.3b0fa4p-4, -0x1.777f26p-12, 0x1.c128ccp-8 }, /* 34 */
  { 0x1.baf7c2p+6, 0x1.baf816p+6, 0x1.baf86cp+6, -0x2.24ccc8p-24,
    -0x1.368f5cp-4, 0x1.62bd9ep-12, 0xa.df002p-8 }, /* 35 */
  { 0x1.c788dap+6, 0x1.c7890cp+6, 0x1.c7896cp+6, 0x4.7dcea8p-24,
    0x1.323f16p-4, -0x1.61abf4p-12, 0xa.ad73ep-8 }, /* 36 */
  { 0x1.d419ccp+6, 0x1.d41a02p+6, 0x1.d41a68p+6, -0x4.b538p-24,
    -0x1.2e1b98p-4, 0x1.4a4d64p-12, 0x3.a47674p-8 }, /* 37 */
  { 0x1.e0aacep+6, 0x1.e0aaf8p+6, 0x1.e0ab5ep+6, 0x3.09dc4cp-24,
    0x1.2a21ecp-4, -0x1.3fa20cp-12, 0x2.216e8cp-8 }, /* 38 */
  { 0x1.ed3bb8p+6, 0x1.ed3beep+6, 0x1.ed3c56p+6, 0x4.d5a58p-28,
    -0x1.264f66p-4, 0x1.32c4cep-12, 0x1.53cbb4p-8 }, /* 39 */
  { 0x1.f9ccaep+6, 0x1.f9cce6p+6, 0x1.f9cd52p+6, 0x3.f4c44cp-24,
    0x1.22a192p-4, -0x1.1f8514p-12, -0xc.0de32p-8 }, /* 40 */
  { 0x1.032ed6p+7, 0x1.032eeep+7, 0x1.032f0cp+7, 0x2.4beae8p-24,
    -0x1.1f1634p-4, 0x1.171664p-12, 0x1.72a654p-4 }, /* 41 */
  { 0x1.097756p+7, 0x1.09776ap+7, 0x1.09779cp+7, -0xd.a581ep-28,
    0x1.1bab3cp-4, -0xf.9f507p-16, -0xc.ba2d4p-8 }, /* 42 */
  { 0x1.0fbfdp+7, 0x1.0fbfe6p+7, 0x1.0fbff6p+7, 0xa.7c0bdp-28,
    -0x1.185eccp-4, 0x1.01d7dep-12, -0x1.a2186ep-4 }, /* 43 */
  { 0x1.160856p+7, 0x1.160862p+7, 0x1.16087ap+7, -0x1.9452ecp-24,
    0x1.152f26p-4, -0x1.07b4aap-12, 0x1.6bbd7ep-4 }, /* 44 */
  { 0x1.1c50dp+7, 0x1.1c50dep+7, 0x1.1c5118p+7, 0x3.83b7b8p-24,
    -0x1.121ab2p-4, 0x1.0e938cp-12, -0x5.1a6dfp-8 }, /* 45 */
  { 0x1.22995p+7, 0x1.22995ap+7, 0x1.229976p+7, -0x6.5ca3a8p-24,
    0x1.0f1ff2p-4, -0xe.f198p-16, -0x3.8e98b8p-8 }, /* 46 */
  { 0x1.28e1ccp+7, 0x1.28e1d8p+7, 0x1.28e1f4p+7, -0x6.bb61ap-24,
    -0x1.0c3d8ap-4, 0xf.a14b9p-16, 0x9.81e82p-4 }, /* 47 */
  { 0x1.2f2a48p+7, 0x1.2f2a54p+7, 0x1.2f2a74p+7, 0x2.2438p-24,
    0x1.097236p-4, -0xd.fed5ep-16, -0x3.19eb5cp-8 }, /* 48 */
  { 0x1.3572b8p+7, 0x1.3572dp+7, 0x1.3572ecp+7, 0x3.1e0054p-24,
    -0x1.06bcc8p-4, 0xd.d2596p-16, -0x1.67e00ap-4 }, /* 49 */
  { 0x1.3bbb3ep+7, 0x1.3bbb4ep+7, 0x1.3bbb6ap+7, 0x7.46c908p-24,
    0x1.041c28p-4, -0xd.04045p-16, -0x8.fb297p-8 }, /* 50 */
  { 0x1.4203aep+7, 0x1.4203cap+7, 0x1.4203e6p+7, -0xb.4f158p-28,
    -0x1.018f52p-4, 0xc.ccf6fp-16, 0x1.4d5dp-4 }, /* 51 */
  { 0x1.484c38p+7, 0x1.484c46p+7, 0x1.484c56p+7, -0x6.5a89c8p-24,
    0xf.f155p-8, -0xc.5d21dp-16, -0xd.aca34p-8 }, /* 52 */
  { 0x1.4e94b8p+7, 0x1.4e94c4p+7, 0x1.4e94d4p+7, -0x1.ef16c8p-24,
    -0xf.cad3fp-8, 0xb.d75f8p-16, 0x1.f36732p-4 }, /* 53 */
  { 0x1.54dd36p+7, 0x1.54dd4p+7, 0x1.54dd52p+7, -0x6.1e7b68p-24,
    0xf.a564cp-8, -0xb.ec1cfp-16, 0xe.e7421p-8 }, /* 54 */
  { 0x1.5b25aep+7, 0x1.5b25bep+7, 0x1.5b25d4p+7, -0xf.8c858p-28,
    -0xf.80faep-8, 0xb.8b6c5p-16, -0x5.835ed8p-8 }, /* 55 */
  { 0x1.616e34p+7, 0x1.616e3cp+7, 0x1.616e4ep+7, 0x7.75d858p-24,
    0xf.5d8abp-8, -0xb.b3779p-16, 0x2.40b948p-4 }, /* 56 */
  { 0x1.67b6bp+7, 0x1.67b6b8p+7, 0x1.67b6dp+7, 0x1.d78632p-24,
    -0xf.3b096p-8, 0xa.daf89p-16, 0x1.aa8548p-8 }, /* 57 */
  { 0x1.6dff28p+7, 0x1.6dff36p+7, 0x1.6dff54p+7, 0x3.b24794p-24,
    0xf.196c7p-8, -0xb.1afe1p-16, -0x1.77538cp-8 }, /* 58 */
  { 0x1.7447a2p+7, 0x1.7447b2p+7, 0x1.7447cap+7, 0x6.39cbc8p-24,
    -0xe.f8aa5p-8, 0xa.50daap-16, 0x1.9592c2p-8 }, /* 59 */
  { 0x1.7a902p+7, 0x1.7a903p+7, 0x1.7a903ep+7, -0x1.820e3ap-24,
    0xe.d8b9dp-8, -0xa.998cp-16, -0x2.c35d78p-4 }, /* 60 */
  { 0x1.80d89ep+7, 0x1.80d8aep+7, 0x1.80d8bep+7, -0x2.c7e038p-24,
    -0xe.b9925p-8, 0x9.ce06p-16, -0x2.2b3054p-4 }, /* 61 */
  { 0x1.87211cp+7, 0x1.87212cp+7, 0x1.872144p+7, 0x6.ab31c8p-24,
    0xe.9b2bep-8, -0x9.4de7p-16, -0x1.32cb5ep-4 }, /* 62 */
  { 0x1.8d699ap+7, 0x1.8d69a8p+7, 0x1.8d69bp+7, 0x4.4ef25p-24,
    -0xe.7d7ecp-8, 0x9.a0f1ep-16, 0x1.6ac076p-4 }, /* 63 */
};

/* Formula page 5 of https://www.cl.cam.ac.uk/~jrh13/papers/bessel.pdf:
   y0(x) ~ sqrt(2/(pi*x))*beta0(x)*sin(x-pi/4-alpha0(x))
   where beta0(x) = 1 - 1/(16*x^2) + 53/(512*x^4)
   and alpha0(x) = 1/(8*x) - 25/(384*x^3).  */
static float
y0f_asympt (float x)
{
  /* The following code fails to give an error <= 9 ulps in only two cases,
     for which we tabulate the correctly-rounded result.  */
  if (x == 0x1.bfad96p+7f)
    return -0x7.f32bdp-32f;
  if (x == 0x1.2e2a42p+17f)
    return 0x1.a48974p-40f;
  double y = 1.0 / (double) x;
  double y2 = y * y;
  double beta0 = 1.0f + y2 * (-0x1p-4f + 0x1.a8p-4 * y2);
  double alpha0 = y * (0x2p-4 - 0x1.0aaaaap-4 * y2);
  double h;
  int n;
  h = reduce_aux (x, &n, alpha0);
  /* Now x - pi/4 - alpha0 = h + n*pi/2 mod (2*pi).  */
  float xr = (float) h;
  n = n & 3;
  float cst = 0xc.c422ap-4; /* sqrt(2/pi) rounded to nearest  */
  float t = cst / sqrtf (x) * (float) beta0;
  if (n == 0)
    return t * __sinf (xr);
  else if (n == 2) /* sin(x+pi) = -sin(x)  */
    return -t * __sinf (xr);
  else if (n == 1) /* sin(x+pi/2) = cos(x)  */
    return t * __cosf (xr);
  else             /* sin(x+3pi/2) = -cos(x)  */
    return -t * __cosf (xr);
}

/* Special code for x near a root of y0.
   z is the value computed by the generic code.
   For small x, use a polynomial approximating y0 around its root.
   For large x, use an asymptotic formula (y0f_asympt).  */
static float
y0f_near_root (float x, float z)
{
  float index_f;
  int index;

  index_f = roundf ((x - FIRST_ZERO_Y0) / M_PIf);
  if (index_f >= SMALL_SIZE)
    return y0f_asympt (x);
  index = (int) index_f;
  const float *p = Py[index];
  float x0 = p[0];
  float x1 = p[2];
  /* If not in the interval [x0,x1] around xmid, return the value z.  */
  if (! (x0 <= x && x <= x1))
    return z;
  float xmid = p[1];
  float y = x - xmid;
  /* For degree 0 use a degree-5 polynomial, where the coefficients of
     degree 4 and 5 are hard-coded.  */
  float p6 = (index > 0) ? p[6]
    : p[6] + y * (-0x3.a46c9p-4 + y * 0x3.735478p-4);
  float res = p[3] + y * (p[4] + y * (p[5] + y * p6));
  return res;
}

float
__ieee754_y0f(float x)
{
	float z, s,c,ss,cc,u,v;
	int32_t hx,ix;

	GET_FLOAT_WORD(hx,x);
	ix = 0x7fffffff&hx;
    /* Y0(NaN) is NaN, y0(-inf) is Nan, y0(inf) is 0, y0(0) is -inf.  */
	if(ix>=0x7f800000) return  one/(x+x*x);
	if(ix==0) return -1/zero; /* -inf and divide by zero exception.  */
	if(hx<0) return zero/(zero*x);
	if(ix >= 0x40000000 || (0x3f5340ed <= ix && ix <= 0x3f77b5e5)) {
          /* |x| >= 2.0 or
	     0x1.a681dap-1 <= |x| <= 0x1.ef6bcap-1 (around 1st zero) */
	/* y0(x) = sqrt(2/(pi*x))*(p0(x)*sin(x0)+q0(x)*cos(x0))
	 * where x0 = x-pi/4
	 *      Better formula:
	 *              cos(x0) = cos(x)cos(pi/4)+sin(x)sin(pi/4)
	 *                      =  1/sqrt(2) * (sin(x) + cos(x))
	 *              sin(x0) = sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
	 *                      =  1/sqrt(2) * (sin(x) - cos(x))
	 * To avoid cancellation, use
	 *              sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
	 * to compute the worse one.
	 */
                SET_RESTORE_ROUNDF (FE_TONEAREST);
		__sincosf (x, &s, &c);
		ss = s-c;
		cc = s+c;
	/*
	 * j0(x) = 1/sqrt(pi) * (P(0,x)*cc - Q(0,x)*ss) / sqrt(x)
	 * y0(x) = 1/sqrt(pi) * (P(0,x)*ss + Q(0,x)*cc) / sqrt(x)
	 */
                if (ix >= 0x7f000000)
		  /* x >= 2^127: use asymptotic expansion.  */
                  return y0f_asympt (x);
                /* Now we are sure that x+x cannot overflow.  */
                z = -__cosf(x+x);
                if ((s*c)<zero) cc = z/ss;
                else            ss = z/cc;
                if (ix <= 0x5c000000)
                  {
                    u = pzerof(x); v = qzerof(x);
                    ss = u*ss+v*cc;
                  }
                z = (invsqrtpi*ss)/sqrtf(x);
                /* The following threshold is optimal (determined on
                   aarch64-linux-gnu).  */
                float threshold = 0x1.be585ap-4;
                if (fabsf (ss) > threshold)
                  return z;
                else
                  return y0f_near_root (x, z);
	}
	if(ix<=0x39800000) {	/* x < 2**-13 */
	    return(u00 + tpi*__ieee754_logf(x));
	}
	z = x*x;
	u = u00+z*(u01+z*(u02+z*(u03+z*(u04+z*(u05+z*u06)))));
	v = one+z*(v01+z*(v02+z*(v03+z*v04)));
	return(u/v + tpi*(__ieee754_j0f(x)*__ieee754_logf(x)));
}
libm_alias_finite (__ieee754_y0f, __y0f)

/* The asymptotic expansion of pzero is
 *	1 - 9/128 s^2 + 11025/98304 s^4 - ...,	where s = 1/x.
 * For x >= 2, We approximate pzero by
 *	pzero(x) = 1 + (R/S)
 * where  R = pR0 + pR1*s^2 + pR2*s^4 + ... + pR5*s^10
 *	  S = 1 + pS0*s^2 + ... + pS4*s^10
 * and
 *	| pzero(x)-1-R/S | <= 2  ** ( -60.26)
 */
static const float pR8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
  0.0000000000e+00, /* 0x00000000 */
 -7.0312500000e-02, /* 0xbd900000 */
 -8.0816707611e+00, /* 0xc1014e86 */
 -2.5706311035e+02, /* 0xc3808814 */
 -2.4852163086e+03, /* 0xc51b5376 */
 -5.2530439453e+03, /* 0xc5a4285a */
};
static const float pS8[5] = {
  1.1653436279e+02, /* 0x42e91198 */
  3.8337448730e+03, /* 0x456f9beb */
  4.0597855469e+04, /* 0x471e95db */
  1.1675296875e+05, /* 0x47e4087c */
  4.7627726562e+04, /* 0x473a0bba */
};
static const float pR5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
 -1.1412546255e-11, /* 0xad48c58a */
 -7.0312492549e-02, /* 0xbd8fffff */
 -4.1596107483e+00, /* 0xc0851b88 */
 -6.7674766541e+01, /* 0xc287597b */
 -3.3123129272e+02, /* 0xc3a59d9b */
 -3.4643338013e+02, /* 0xc3ad3779 */
};
static const float pS5[5] = {
  6.0753936768e+01, /* 0x42730408 */
  1.0512523193e+03, /* 0x44836813 */
  5.9789707031e+03, /* 0x45bad7c4 */
  9.6254453125e+03, /* 0x461665c8 */
  2.4060581055e+03, /* 0x451660ee */
};

static const float pR3[6] = {/* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
 -2.5470459075e-09, /* 0xb12f081b */
 -7.0311963558e-02, /* 0xbd8fffb8 */
 -2.4090321064e+00, /* 0xc01a2d95 */
 -2.1965976715e+01, /* 0xc1afba52 */
 -5.8079170227e+01, /* 0xc2685112 */
 -3.1447946548e+01, /* 0xc1fb9565 */
};
static const float pS3[5] = {
  3.5856033325e+01, /* 0x420f6c94 */
  3.6151397705e+02, /* 0x43b4c1ca */
  1.1936077881e+03, /* 0x44953373 */
  1.1279968262e+03, /* 0x448cffe6 */
  1.7358093262e+02, /* 0x432d94b8 */
};

static const float pR2[6] = {/* for x in [2.8570,2]=1/[0.3499,0.5] */
 -8.8753431271e-08, /* 0xb3be98b7 */
 -7.0303097367e-02, /* 0xbd8ffb12 */
 -1.4507384300e+00, /* 0xbfb9b1cc */
 -7.6356959343e+00, /* 0xc0f4579f */
 -1.1193166733e+01, /* 0xc1331736 */
 -3.2336456776e+00, /* 0xc04ef40d */
};
static const float pS2[5] = {
  2.2220300674e+01, /* 0x41b1c32d */
  1.3620678711e+02, /* 0x430834f0 */
  2.7047027588e+02, /* 0x43873c32 */
  1.5387539673e+02, /* 0x4319e01a */
  1.4657617569e+01, /* 0x416a859a */
};

static float
pzerof(float x)
{
	const float *p,*q;
	float z,r,s;
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	/* ix >= 0x40000000 for all calls to this function.  */
	if(ix>=0x41000000)     {p = pR8; q= pS8;}
	else if(ix>=0x40f71c58){p = pR5; q= pS5;}
	else if(ix>=0x4036db68){p = pR3; q= pS3;}
	else {p = pR2; q= pS2;}
	z = one/(x*x);
	r = p[0]+z*(p[1]+z*(p[2]+z*(p[3]+z*(p[4]+z*p[5]))));
	s = one+z*(q[0]+z*(q[1]+z*(q[2]+z*(q[3]+z*q[4]))));
	return one+ r/s;
}


/* For x >= 8, the asymptotic expansion of qzero is
 *	-1/8 s + 75/1024 s^3 - ..., where s = 1/x.
 * We approximate pzero by
 *	qzero(x) = s*(-1.25 + (R/S))
 * where  R = qR0 + qR1*s^2 + qR2*s^4 + ... + qR5*s^10
 *	  S = 1 + qS0*s^2 + ... + qS5*s^12
 * and
 *	| qzero(x)/s +1.25-R/S | <= 2  ** ( -61.22)
 */
static const float qR8[6] = { /* for x in [inf, 8]=1/[0,0.125] */
  0.0000000000e+00, /* 0x00000000 */
  7.3242187500e-02, /* 0x3d960000 */
  1.1768206596e+01, /* 0x413c4a93 */
  5.5767340088e+02, /* 0x440b6b19 */
  8.8591972656e+03, /* 0x460a6cca */
  3.7014625000e+04, /* 0x471096a0 */
};
static const float qS8[6] = {
  1.6377603149e+02, /* 0x4323c6aa */
  8.0983447266e+03, /* 0x45fd12c2 */
  1.4253829688e+05, /* 0x480b3293 */
  8.0330925000e+05, /* 0x49441ed4 */
  8.4050156250e+05, /* 0x494d3359 */
 -3.4389928125e+05, /* 0xc8a7eb69 */
};

static const float qR5[6] = { /* for x in [8,4.5454]=1/[0.125,0.22001] */
  1.8408595828e-11, /* 0x2da1ec79 */
  7.3242180049e-02, /* 0x3d95ffff */
  5.8356351852e+00, /* 0x40babd86 */
  1.3511157227e+02, /* 0x43071c90 */
  1.0272437744e+03, /* 0x448067cd */
  1.9899779053e+03, /* 0x44f8bf4b */
};
static const float qS5[6] = {
  8.2776611328e+01, /* 0x42a58da0 */
  2.0778142090e+03, /* 0x4501dd07 */
  1.8847289062e+04, /* 0x46933e94 */
  5.6751113281e+04, /* 0x475daf1d */
  3.5976753906e+04, /* 0x470c88c1 */
 -5.3543427734e+03, /* 0xc5a752be */
};

static const float qR3[6] = {/* for x in [4.547,2.8571]=1/[0.2199,0.35001] */
  4.3774099900e-09, /* 0x3196681b */
  7.3241114616e-02, /* 0x3d95ff70 */
  3.3442313671e+00, /* 0x405607e3 */
  4.2621845245e+01, /* 0x422a7cc5 */
  1.7080809021e+02, /* 0x432acedf */
  1.6673394775e+02, /* 0x4326bbe4 */
};
static const float qS3[6] = {
  4.8758872986e+01, /* 0x42430916 */
  7.0968920898e+02, /* 0x44316c1c */
  3.7041481934e+03, /* 0x4567825f */
  6.4604252930e+03, /* 0x45c9e367 */
  2.5163337402e+03, /* 0x451d4557 */
 -1.4924745178e+02, /* 0xc3153f59 */
};

static const float qR2[6] = {/* for x in [2.8570,2]=1/[0.3499,0.5] */
  1.5044444979e-07, /* 0x342189db */
  7.3223426938e-02, /* 0x3d95f62a */
  1.9981917143e+00, /* 0x3fffc4bf */
  1.4495602608e+01, /* 0x4167edfd */
  3.1666231155e+01, /* 0x41fd5471 */
  1.6252708435e+01, /* 0x4182058c */
};
static const float qS2[6] = {
  3.0365585327e+01, /* 0x41f2ecb8 */
  2.6934811401e+02, /* 0x4386ac8f */
  8.4478375244e+02, /* 0x44533229 */
  8.8293585205e+02, /* 0x445cbbe5 */
  2.1266638184e+02, /* 0x4354aa98 */
 -5.3109550476e+00, /* 0xc0a9f358 */
};

static float
qzerof(float x)
{
	const float *p,*q;
	float s,r,z;
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	/* ix >= 0x40000000 for all calls to this function.  */
	if(ix>=0x41000000)     {p = qR8; q= qS8;}
	else if(ix>=0x40f71c58){p = qR5; q= qS5;}
	else if(ix>=0x4036db68){p = qR3; q= qS3;}
	else {p = qR2; q= qS2;}
	z = one/(x*x);
	r = p[0]+z*(p[1]+z*(p[2]+z*(p[3]+z*(p[4]+z*p[5]))));
	s = one+z*(q[0]+z*(q[1]+z*(q[2]+z*(q[3]+z*(q[4]+z*q[5])))));
	return (-(float).125 + r/s)/x;
}
