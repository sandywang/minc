/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#ifndef UD_GENERIC_H
#define UD_GENERIC_H

union generic {			/* used to hold any kind of fill_value */
    float floatv;
    double doublev;
    int intv;
    short shortv;
    char charv;
};

#endif
