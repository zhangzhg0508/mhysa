Input Files
===========

The following input files are required to run HyPar. They are indicated as being mandatory or 
optional. If optional, then default values are used if not found.

\b Note: It is best to start with the provided examples, understand their input files based on the 
information on this page, and then modify them to run other cases.

\section solver_inp solver.inp

Requirement: \b mandatory

Read by: ReadInputs()

Description: Specify the main simulation-related parameters.

Format: ASCII text

        begin
            <keyword>   <value>
            <keyword>   <value>
            <keyword>   <value>
            ...
            <keyword>   <value>
        end

where the list of keywords and their type are:\n
Keyword name       | Type         | Variable                      | Default value
------------------ | ------------ | ----------------------------- | ------------------------
ndims              | int          | #HyPar::ndims                 | 1
nvars              | int          | #HyPar::nvars                 | 1
size               | int[ndims]   | #HyPar::dim_global            | must be specified
iproc              | int[ndims]   | #MPIVariables::iproc          | must be specified
ghost              | int          | #HyPar::ghosts                | 1
n_iter             | int          | #HyPar::n_iter                | 0
restart_iter       | int          | #HyPar::restart_iter          | 0
time_scheme        | char[]       | #HyPar::time_scheme           | euler
time_scheme_type   | char[]       | #HyPar::time_scheme_type      | none
hyp_space_scheme   | char[]       | #HyPar::spatial_scheme_hyp    | 1
hyp_flux_split     | char[]       | #HyPar::SplitHyperbolicFlux   | no
hyp_interp_type    | char[]       | #HyPar::interp_type           | characteristic
par_space_type     | char[]       | #HyPar::spatial_type_par      | nonconservative-1stage
par_space_scheme   | char[]       | #HyPar::spatial_scheme_par    | 2
dt                 | double       | #HyPar::dt                    | 0.0
conservation_check | char[]       | #HyPar::ConservationCheck     | no
screen_op_iter     | int          | #HyPar::screen_op_iter        | 1
file_op_iter       | int          | #HyPar::file_op_iter          | 1000
op_file_format     | char[]       | #HyPar::op_file_format        | text
ip_file_type       | char[]       | #HyPar::ip_file_type          | ascii
input_mode         | char[]       | #HyPar::input_mode            | serial
output_mode        | char[]       | #HyPar::output_mode           | serial
op_overwrite       | char[]       | #HyPar::op_overwrite          | no
model              | char[]       | #HyPar::model                 | must be specified

\b Notes:
+ "ndims" \b must be specified \b before "size" and "iproc".
+ if "input_mode" or "output_mode" are set to "parallel" or "mpi-io",
  the number of I/O ranks must be specified right after as an integer.
  For example:

      begin
          ...
          input_mode  parallel 4
          ...
      end

  This means that 4 MPI ranks will participate in file I/O (assuming
  total MPI ranks is more than 4) (see ReadArrayParallel(), 
  WriteArrayParallel(), ReadArrayMPI_IO() ).
  - The number of I/O ranks specified for "input_mode" and "output_mode"
    \b must \b be \b same. Otherwise, the value for the one specified last
    will be used.
  - The number of I/O ranks must be such that the total number of MPI ranks
    is an integer multiple. Otherwise, the code will use only 1 I/O rank.
+ If any of the keywords are not present, the default value is used, except
  the ones whose default values say "must be specified". Thus, keywords that
  are not required for a particular simulation may be left out of the 
  solver.inp input file. For example, 
  - a #Euler1D simulation does not need "par_space_type" or "par_space_scheme"
    because it does not have a parabolic term.
  - unless a conservation check is required, "conservation_check" can be left
    out and the code will not check for conservation.

\section boundary_inp boundary.inp

Requirement: \b mandatory

Read by: InitializeBoundaries()

Description: Specify boundary conditions.

Format: ASCII text
        
        nb
        boundary_type   spatial_dimension   face   [extent] 
        boundary_type   spatial_dimension   face   [extent] 
        ...
        boundary_type   spatial_dimension   face   [extent] 

where
+ \b nb is the number of boundaries
+ This is followed by at least \b nb rows, each row specifying:
  - \b boundary_type - Type of boundary, eg. #_PERIODIC_, #_EXTRAPOLATE_, #_DIRICHLET_, etc. See include/boundaryconditions.h for a full list.
  - \b spatial_dimension - The spatial dimension along which this boundary condition applies. Must be in the range 0, 1, ..., #HyPar::ndims-1
  - \b face - The face on which this the boundary acts (1 means the left face along the dimension, or the low index end, -1 means the right face
              along the dimension, or the high index end).
  - [\b extent] - The extent of this boundary (in terms of coordinates, not grid indices). Written as (2 X #HyPar::ndims) real numbers:
                  xmin_0  xmax_0  xmin_1  xmax_1 ... ... xmin_(ndims-1)  xmax_(ndims-1), where [xmin_i, xmax_i] define the extent along
                  spatial dimension i. Note that the xmin_i and xmax_i for i = spatial_dimension are ignored, but should be there for the
                  sake of a uniform format (they can be set as 0).
+ For the following boundary types, an additional line is required right below the line specifying that boundary with inputs specific to that 
  boundary type:
  - #_DIRICHLET_: the next line should specify the boundary value of the solution at that boundary (the value is assumed to be constant in space
    and time). Each component of the solution must be specified (i.e. #HyPar::nvars real numbers).

        u[0]  u[1]  ... u[#HyPar::nvars-1]

  - #_SLIP_WALL_, #_NOSLIP_WALL_, #_SW_SLIP_WALL_, or #_SW_NOSLIP_WALL_: the next line should specify the wall velocity (assumed to be constant 
    in space and time). Each component of the velocity must be specified (i.e. #HyPar::ndims real numbers).

        u[0]  u[1]  ... u[#HyPar::ndims-1]

  - #_SUBSONIC_INFLOW_: the next line must specify the inflow density and velocity vector (assumed to be constant in space and time). Each component
    of the velocity must be specified (i.e. #HyPar::ndims+1 real numbers)

        rho  u[0]  u[1]  ... u[#HyPar::ndims-1]

  - #_SUBSONIC_OUTFLOW_: the next line must specify the outflow pressure (assumed to be constant in space and time), i.e., one real number

        p

  - #_SUPERSONIC_INFLOW_: the next line must specify the inflow density, velocity, and pressure (assumed to be constant in space and time). Each component
    of the velocity must be specified (i.e. #HyPar::ndims+2 real numbers)

        rho  u[0]  u[1]  ... u[#HyPar::ndims-1]  p

    Note that this boundary can also be implemented through the #_DIRICHLET_; however, the flow variables must be converted to the conserved variable
    form to specify the Dirichlet boundary value:

        rho  rho*u[0]  rho*u[1]  ... rho*u[#HyPar::ndims-1]  E (internal energy)

  - #_TURBULENT_SUPERSONIC_INFLOW_: the next line must specify the inflow mean density, velocity, and pressure (assumed to be constant in space and time),
    i.e, #HyPar::ndims+2 real numbersm and the name of the file that contains the unsteady fluctuation data.

        rho  u[0]  u[1]  ... u[#HyPar::ndims-1]  p  filename


\section initial_inp initial.inp

Requirement: \b mandatory

Read by: InitialSolution(), through ReadArray()

Format: Depends on #HyPar::input_mode and #HyPar::ip_file_type, specified in \b solver.inp. See ReadArraySerial(), ReadArrayParallel(), and 
        ReadArrayMPI_IO() to understand the format and data layout of this file.

Description: This file contains the initial solution. It will probably not be created by hand. See the examples provided for codes that generate
             this file for various problems. The final part in these codes, where the initial solution is written to file, can be reused to 
             generate this file for a new case.

\section lusolver_inp lusolver.inp

Requirement: \b optional

Read by: tridiagLUInit()

Description: Specify parameters related to LU solvers for tridiagonal systems of equations. This file is relevant only if
             some method requiring the solution of a tridiagonal system of equations is being used (for example:
             Interp1PrimFifthOrderCRWENO() ).

Format: ASCII text
        
        begin
            <keyword>   <value>
            <keyword>   <value>
            <keyword>   <value>
            ...
            <keyword>   <value>
        end

where the list of keywords are:

Keyword name       | Type         | Variable                      | Default value
------------------ | ------------ | ----------------------------- | ------------------------
evaluate_norm      | int          | #TridiagLU::evaluate_norm     | 1
maxiter            | int          | #TridiagLU::maxiter           | 10
atol               | double       | #TridiagLU::atol              | 1e-12
rtol               | double       | #TridiagLU::rtol              | 1e-10
verbose            | int          | #TridiagLU::verbose           | 0
reducedsolvetype   | char[]       | #TridiagLU::reducedsolvetype  | #_TRIDIAG_JACOBI_


\section weno_inp weno.inp

Requirement: \b optional

Read by: WENOInitialize() 

Description: Specify parameters related to WENO-type spatial discretization. This file is relevant only if a WENO-type 
             method is being used for spatial discretization (for example: Interp1PrimFifthOrderCRWENO(), Interp1PrimFifthOrderWENO(),
             Interp1PrimFifthOrderHCWENO() ). For most cases, this file is useful if a very specific study on the behavior of the
             WENO-type method is being carried out; typically, the default values are enough to ensure "good" solutions.

Format: ASCII text
        
        begin
            <keyword>   <value>
            <keyword>   <value>
            ...
            <keyword>   <value>
        end

where the list of keywords are:

Keyword name       | Type         | Variable                      | Default value
------------------ | ------------ | ----------------------------- | ------------------------
mapped             | int          | #WENOParameters::mapped       | 0
borges             | int          | #WENOParameters::borges       | 0
yc                 | int          | #WENOParameters::yc           | 0
no_limiting        | int          | #WENOParameters::no_limiting  | 0
epsilon            | double       | #WENOParameters::eps          | 1e-6
rc                 | double       | #WENOParameters::rc           | 0.3
xi                 | double       | #WENOParameters::xi           | 0.001
tol                | double       | #WENOParameters::tol          | 1e-16

\section physics_inp physics.inp

Requirement: \b mandatory/optional (depends on the physical model)

Read by: The initialization function of the various physics modules (eg. Euler1DInitialize(), NavierStokes2DInitialize(), etc)

Description: This file contains inputs specific to a physics model. Depending on the physical model being used, it may or may not
             be mandatory. The exact parameters to specify depend on the physics.

Format: ASCII text
        
        begin
            <keyword>   <value>
            <keyword>   <value>
            ...
            <keyword>   <value>
        end

See the documentation for the initialization function of the various phyical models for a list of keywords (for example, Euler1DInitialize(), NavierStokes2DInitialize()).