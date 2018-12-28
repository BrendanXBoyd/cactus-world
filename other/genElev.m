%Brendan Boyd   CSCI5229    Project
%This code generates an elevation map for the project
clear all; close all; clc;

%Create a grid of (x,y) coordinates to generate z for
Lims = 20;
N = 256+1;
step = 2*Lims/(N-1);

X = -Lims:step:Lims;
Y = -Lims:step:Lims;
Z = zeros(length(X),length(Y));

%Now iterate over each point
for i = 1:length(X)
    for j = 1:length(Y)
        %get x and y
        x = X(i);
        y = Y(j);
        
        %Calculate location in polar
        th = atan2(y,x);
        r = sqrt(x^2 + y^2);
        
        %Calculate z based on r and th
        z1 = exp(-0.1 * (r-12)^2) * (cos(th*5)+1);
        z2 = exp(-0.05* (r-14)^2) * (sin(th*3)+1);
        z3 = exp(-0.03* (r-16)^2) * (sin(th+pi)+1);
        z4 = exp(-0.03* (r-16)^2) * (sin(th)+1);
        z5 = exp(-0.1* (r-20)^2) * (cos(th*2)/3 +1);
        Z(i,j) = 0.3*z1 + 0.3*z2 + 0.4*z3 + 0.4*z4 + 0.4*z5 +1;
        
    end
end

mesh(Z);
dlmwrite('DEM.dem',Z,' ');