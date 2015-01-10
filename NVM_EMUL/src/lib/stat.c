#include "stat.h"
#include <math.h>

double sum(double array[], int n)
{
    int i;
    double s = 0;

    for (i=0; i<n; i++) {
        s += array[i];
    }
    return s;
}

// returns sum of x . y
double sumxy(double x[], double y[], int n)
{
    int i;
    double s = 0;

    for (i=0; i<n; i++) {
        s += x[i] * y[i];
    }
    return s;
}


double avg(double array[], int n)
{
    double s;

    s = sum(array, n);
    return s/n;
}

double slope(double x[], double y[], int n)
{
    double sumxy_;
    double sumx2;
    double sumx;
    double sumy;
    double m; 

    sumxy_ = sumxy(x, y, n);
    sumx2 = sumxy(x, x, n);
    sumx = sum(x, n);
    sumy = sum(y, n);

    m = (n * sumxy_ - sumx * sumy) / 
        (n * sumx2 - sumx*sumx);
    return m;
}
