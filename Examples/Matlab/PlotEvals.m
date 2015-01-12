% This script plots of the eigenvalues computed by the 
% ComputeEvals.m script. It saves the plots to a multi-page
% Postscript file ("Eigenvalues.ps").

clear all;
close all;

MaxFiles = 10000;
fname_FFunction_root = 'Mat_FFunction_';
fname_extn = '.dat';
nevals = input('Enter number of eigenvalues computed: ');

scrsz = get(0,'ScreenSize');
width = scrsz(4)/2;
height = 0.9*width;
EigPlot = figure('Position',[1 scrsz(4)/2 width height]);

nplot = 0;
for i=1:MaxFiles
    index = sprintf('%05d',i-1);
    filename_FFunction = strcat(fname_FFunction_root,index,fname_extn);
    str_nevals   = strcat(sprintf('%05d',nevals),'_');
    Feval_fname  = strcat('EVals_',str_nevals,filename_FFunction);

    if (exist(Feval_fname,'file'))
        fprintf('Plotting FFunction eigenvalues from  %s\n',Feval_fname);
        dataF  = load(Feval_fname);
        figure(EigPlot);
        plot(dataF(:,2),dataF(:,3),'bo');
        hold on;
        flagFFunction = 1;
        legend_str = 'F(u)';
    else
        flagFFunction = 0;
    end
    if (flagFFunction)
        figure(EigPlot);
        set(gca,'FontSize',10,'FontName','Times');
        xlabel('Real(\lambda)','FontName','Times','FontSize',14, ...
            'FontWeight','normal');
        ylabel('Imag(\lambda)','FontName','Times','FontSize',14, ...
            'FontWeight','normal');
        legend(legend_str,'Location','northwest');
        legend('boxoff');
        title(index);
        axis([1.1*min([min(dataF(:,2)),-1.0]), ...
            1.1*max([max(dataF(:,2)), 0.1]), ...
            1.1*min([min(dataF(:,3)),-1.0]), ...
            1.1*max([max(dataF(:,3)), 0.1])]);
        grid on;
        hold off;
        figname = 'Eigenvalues.ps';
        if (nplot)
            print('-dpsc2',figname,'-append',EigPlot);
        else
            print('-dpsc2',figname,EigPlot);
        end
        nplot = nplot + 1;
    end
    if (~flagFFunction)
        break;
    end
end