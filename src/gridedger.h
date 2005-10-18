
/* Michael A. Park
 * Computational AeroSciences Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:Mike.Park@NASA.Gov 
 */
  
/* $Id$ */

#ifndef GRIDEDGER_H
#define GRIDEDGER_H

#include "refine_defs.h"
#include "grid.h"

BEGIN_C_DECLORATION

typedef struct GridEdger GridEdger;
struct GridEdger {
  Grid *grid;
  void *gridRubyVALUEusedForGC;
};

GridEdger *gridedgerCreate(Grid *);
Grid *gridedgerGrid(GridEdger *);
void gridedgerFree(GridEdger *);
void gridedgerPack(void *voidGridEdger, 
		  int nnode, int maxnode, int *nodeo2n,
		  int ncell, int maxcell, int *cello2n,
		  int nface, int maxface, int *faceo2n,
		  int nedge, int maxedge, int *edgeo2n);
void gridedgerSortNode(void *voidGridEdger, int maxnode, int *o2n);
void gridedgerReallocator(void *voidGridEdger, int reallocType, 
			 int lastSize, int newSize);
void gridedgerGridHasBeenFreed(void *voidGridEdger );

END_C_DECLORATION

#endif /* GRIDEDGER_H */
