#
//
//	This LUT implementation of atan2 is a C++ translation of
//	a Java discussion on the net
//	http://www.java-gaming.org/index.php?topic=14647.0

#ifndef		__COMP_ATAN_H
#define		__COMP_ATAN_H

#include	<math.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<cstdlib>
#include	<limits>
#include	"fm-constants.h"
#
class	compAtan {
public:
		compAtan	();
		~compAtan	();
	float	atan2		(float, float);
	float	argX		(DSPCOMPLEX);
private:
	float	*ATAN2_TABLE_PPY;
	float	*ATAN2_TABLE_PPX;
	float	*ATAN2_TABLE_PNY;
	float	*ATAN2_TABLE_PNX;
	float	*ATAN2_TABLE_NPY;
	float	*ATAN2_TABLE_NPX;
	float	*ATAN2_TABLE_NNY;
	float	*ATAN2_TABLE_NNX;
	float	Stretch;
};

#endif
