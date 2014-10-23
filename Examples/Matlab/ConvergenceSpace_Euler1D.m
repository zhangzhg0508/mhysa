% Script to test the spatial convergence rate
% for a smooth solution of the 1D inviscid Euler
% equations

clear all;
close all;

% remove all useless files
system('rm -rf *.dat *.inp *.log INIT');

fprintf('Spatial convergence test on a smooth solution ');
fprintf('to the 1D inviscid Euler equations.\n');

% Ask for path to HyPar source directory
path = input('Enter path to HyPar source: ','s');

% Compile the code to generate the initial solution
cmd = ['g++ ',path,'/Examples/1D/Euler1D/DensitySineWave/aux/init.C ', ...
    '-o INIT'];
system(cmd);
% find the HyPar binary
hypar = [path,'/bin/HyPar'];

% Get the default
[~,~,~,~,~,~,~,~,~,hyp_flux_split,hyp_int_type,par_type,par_scheme,~, ...
 cons_check,screen_op_iter,file_op_iter,~, ~,input_mode, ...
 output_mode,n_io,op_overwrite,~,nb,bctype,dim,face,limits, ...
 mapped,borges,yc,nl,eps,p,rc,xi,lutype,norm,maxiter,atol,rtol, ...
 verbose] = SetDefaults();

% set problem specific input parameters
ndims = 1;
nvars = 3;
iproc = 1;
ghost = 3;

% specify spatial discretization scheme
hyp_scheme = 'weno5';
fprintf('Spatial discretization scheme: ''%s''\n',hyp_scheme);

% for spatial convergence, use very small time step
dt = 0.0001;
niter = int32(1.0/dt);

% set physical model and related parameters
model = 'euler1d';
gamma = 1.4;
grav  = 0.0;
upw   = 'roe';

% set initial solution file type to ascii
ip_type = 'ascii';

% set time-integration scheme
ts = 'rk';
tstype = '44';

% turn off solution output to file
op_format = 'none';

% set number of grid refinement levels
ref_levels = 5;

% set initial grid size;
N = 20;

% set the commands to run the executables
nproc = 1;
for i = 1:max(size(iproc))
    nproc = nproc * iproc(i);
end
init_exec = './INIT > init.log 2>&1';
hypar_exec = ['$MPI_DIR/bin/mpiexec -n ',num2str(nproc),' ',hypar, ...
               ' > run.log 2>&1'];
clean_exec = 'rm -rf *.inp *.dat *.log';

% preallocate arrays for dx, error and wall times
dx = zeros(ref_levels,1);
Errors = zeros(ref_levels,3);
Walltimes = zeros(ref_levels,2);

% convergence analysis
for r = 1:ref_levels
    dx(r) = 1.0 / N;
    fprintf('\t%2d:  N=%-5d   dx=%1.16e\n',r,N,dx(r));
    % Write out the input files for HyPar
    WriteSolverInp(ndims,nvars,N,iproc,ghost,niter,ts,tstype, ...
        hyp_scheme,hyp_flux_split,hyp_int_type,par_type,par_scheme, ...
        dt,cons_check,screen_op_iter,file_op_iter,op_format,ip_type, ...
        input_mode,output_mode,n_io,op_overwrite,model);
    WriteBoundaryInp(nb,bctype,dim,face,limits);
    WritePhysicsInp_Euler1D(gamma,grav,upw);
    WriteWenoInp(mapped,borges,yc,nl,eps,p,rc,xi);
    WriteLusolverInp(lutype,norm,maxiter,atol,rtol,verbose);
    % Generate the initial and exact solution
    system(init_exec);
    system('ln -sf initial.inp exact.inp');
    % Run HyPar
    system(hypar_exec);
    % Read in the errors
    [Errors(r,:),Walltimes(r,:)] = ReadErrorDat(ndims);
    % Clean up
    system(clean_exec);
    % increase grid size
    N = 2*N;
end

% open figure window
scrsz = get(0,'ScreenSize');
figErrDx   = figure('Position',[1 scrsz(4)/2 scrsz(3)/2 scrsz(4)/2]);

% set title string
title_str = ['Spatial convergence plot for ''',hyp_scheme,''''];

% plot errors
loglog(dx,Errors(:,1),'-ko','linewidth',2,'MarkerSize',10);
hold on;
loglog(dx,Errors(:,2),'-ks','linewidth',2,'MarkerSize',10);
hold on;
loglog(dx,Errors(:,3),'-kd','linewidth',2,'MarkerSize',10);
hold on;
xlabel('dx','FontName','Times','FontSize',20,'FontWeight','normal');
ylabel('Error','FontName','Times','FontSize',20,'FontWeight','normal');
set(gca,'FontSize',14,'FontName','Times');
legend(['L1  ';'L2  ';'Linf'],'Location','northwest');
title(title_str);
grid on;

% clean up
system('rm -rf INIT');