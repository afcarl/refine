
/* Computes metrics from faces and tets 
 *
 * Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include "gridmpi.h"

Grid *gridIdentityGlobal(Grid *grid )
{
  int node;
  for (node = 0; node < gridNNode(grid) ; node++ )
    if (grid != gridSetNodeGlobal(grid,node,node)) return NULL;
  return grid;
}

Grid *gridSetAllLocal(Grid *grid )
{
  return grid;
}
