#include <physicalmodels/linearadr.h>
#include <mpivars.h>
#include <hypar.h>

double LinearADRComputeCFL(void *s,void *m,double dt,double t)
{
  HyPar         *solver = (HyPar*)        s;
  LinearADR     *params = (LinearADR*)    solver->physics;
  int           d, i, v;

  int     ndims  = solver->ndims;
  int     nvars  = solver->nvars;
  int     ghosts = solver->ghosts;
  int     *dim   = solver->dim_local;
  double  *dxinv = solver->dxinv;

  int     offset  = 0;
  double  max_cfl = 0;
  for (d = 0; d < ndims; d++) {
    for (i = 0; i < dim[d]; i++) {
      for (v = 0; v < nvars; v++) {
        double local_cfl = params->a[nvars*d+v]*dt*dxinv[offset+ghosts+i];
        if (local_cfl > max_cfl) max_cfl = local_cfl;
      }
    }
    offset += (dim[d]+2*ghosts);
  }

  return(max_cfl);
}
