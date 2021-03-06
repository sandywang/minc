/* ----------------------------- MNI Header -----------------------------------
@NAME       : isotope_list.h
@DESCRIPTION: Header file containing list of isotopes and their half-lives.
@METHOD     : 
@GLOBALS    : isotope_list
@CALLS      : 
@CREATED    : January 13, 1993 (Peter Neelin)
@MODIFIED   : 
 * $Log: isotope_list.h,v $
 * Revision 6.1  1999-10-29 17:52:07  neelin
 * Fixed Log keyword
 *
 * Revision 6.0  1997/09/12 13:23:31  neelin
 * Release of minc version 0.6
 *
 * Revision 5.0  1997/08/21  13:24:32  neelin
 * Release of minc version 0.5
 *
 * Revision 4.0  1997/05/07  20:00:13  neelin
 * Release of minc version 0.4
 *
 * Revision 3.0  1995/05/15  19:31:05  neelin
 * Release of minc version 0.3
 *
 * Revision 2.0  1994/09/28  10:33:45  neelin
 * Release of minc version 0.2
 *
 * Revision 1.3  94/09/28  10:33:32  neelin
 * Pre-release
 * 
 * Revision 1.2  93/08/11  15:27:48  neelin
 * Added RCS logging in source.
 * 
---------------------------------------------------------------------------- */

struct {
   char *name;
   double half_life;      /* Half life in seconds */
} isotope_list[] = {
   {"GA-68", 68.3*60.0},
   {"C-11", 20.3*60.0},
   {"F-18", 109.8*60.0},
   {"O-15", 2.03*60.0},
   {"N-13", 9.96*60.0},
   {"GE-68", 414720*60.0},
   {"CU-64", 762*60.0},
   {"CS-137", 999999*60.0},
   {NULL, -1.0}
};
