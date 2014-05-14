/*
 *
 * 3D Nonhydrostatic Unified Model of the Atmosphere (NUMA)
 *
 * References:
 *
 * F.X. Giraldo, M. Restelli, and M. Laeuter, "Semi-Implicit 
 * Formulations of the Euler Equations: Applications to 
 * Nonhydrostatic Atmospheric Modeling", SIAM J. Sci. Comp., 
 * Vol. 32, 3394-3425 (2010)
 *
 * J.F. Kelly and F.X. Giraldo, "Continuous and Discontinuous 
 * Galerkin Methods for a Scalable 3D Nonhydrostatic Atmospheric 
 * Model:  limited-area mode", J. Comp. Phys., Vol. 231, 7988-8008 
 * (2012)
 *
*/

#include <basic.h>
#include <mathfunctions.h>

#define _NUMA3D_ "numa3d"

/* define ndims and nvars for this model */
#undef _MODEL_NDIMS_
#undef _MODEL_NVARS_
#define _MODEL_NDIMS_ 3
#define _MODEL_NVARS_ 5

/* grid directions */
#define _XDIR_ 0
#define _YDIR_ 1
#define _ZDIR_ 2

#define _Numa3DGetFlowVars_(u,drho,uvel,vvel,wvel,dT,rho0) \
  { \
    drho = u[0]; \
    double rho_total = drho+rho0; \
    uvel = u[1]/rho_total; \
    vvel = u[2]/rho_total; \
    wvel = u[3]/rho_total; \
    dT   = u[4]; \
  }

#define _Numa3DSetFlux_(f,dir,drho,uvel,vvel,wvel,dT,dP,rho0) \
  { \
    double rho_total = rho0+drho; \
    if (dir == _XDIR_) { \
      f[0] = rho_total * uvel; \
      f[1] = rho_total*uvel*uvel + dP; \
      f[2] = rho_total*uvel*vvel; \
      f[3] = rho_total*uvel*wvel; \
      f[4] = uvel*dT; \
    } else if (dir == _YDIR_) { \
      f[0] = rho_total * vvel; \
      f[1] = rho_total*uvel*vvel; \
      f[2] = rho_total*vvel*vvel + dP; \
      f[3] = rho_total*wvel*vvel; \
      f[4] = vvel*dT; \
    } else if (dir == _ZDIR_) { \
      f[0] = rho_total * wvel; \
      f[1] = rho_total*uvel*wvel; \
      f[2] = rho_total*vvel*wvel; \
      f[3] = rho_total*wvel*wvel + dP; \
      f[4] = wvel*dT; \
    } \
  }

#define _Numa3DSetSource_(s,param,uvel,vvel,drho,rho0) \
  { \
    s[0] =  0.0; \
    s[1] =  2.0*param->Omega*vvel*(rho0+drho); \
    s[2] = -2.0*param->Omega*uvel*(rho0+drho); \
    s[3] = -param->g*drho; \
    s[4] =  0.0; \
  }

#define _Numa3DComputePressure_(params,T0,dT,P0,dP) \
  { \
    double gamma    = params->gamma; \
    double Pref     = params->Pref; \
    double R        = params->R; \
    double P_total  = Pref * raiseto((R*(T0+dT)/Pref),gamma); \
    dP  = P_total - P0; \
  }

#define _Numa3DComputeSpeedofSound_(gamma,P0,dP,rho0,drho,c) \
  { \
    c = sqrt(gamma*(P0+dP)/(rho0+drho)); \
  }

typedef struct numa3d_parameters {
  double  gamma;      /* Ratio of heat capacities       */
  double  R;          /* Universal gas constant         */
  double  Omega;      /* Angular speed of Earth         */
  double  g;          /* acceleration due to gravity    */
  int     init_atmos; /* choice of initial atmosphere   */

  /* pressure & temperature at zero altitude */
  double Pref, Tref;

  /* mean hydrostatic flow variables */
  double *rho0, *P0, *T0;
} Numa3D;

int Numa3DInitialize (void*,void*);
int Numa3DCleanup    (void*);