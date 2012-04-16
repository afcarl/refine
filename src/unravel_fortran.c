
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:mike.park@nasa.gov
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "unravel_fortran.h"

#include "grid.h"
#include "gridcad.h"
#include "gridmetric.h"

static Grid *grid = NULL;

void FC_FUNC_(unravel_start,UNRAVEL_START)( int *unravel_api_version, int *status )
{
  *unravel_api_version = 1;
  *status = 0;

  if (NULL != grid) gridFree(grid);

  grid = gridCreate(200, 200, 0, 0);

  if (NULL == grid) *status = 1;
}

void FC_FUNC_(unravel_tet,UNRAVEL_TET)( int *c2n, double *x, double *y, double *z, int *status )
{
  int cell_node;
  int local_nodes[4];

  *status = 0;

  for ( cell_node = 0 ; cell_node < 4 ; cell_node++ )
    {
      local_nodes[cell_node] = gridGlobal2Local(grid, c2n[cell_node]-1 );
      if ( EMPTY == local_nodes[cell_node] )
	{
	  local_nodes[cell_node] = 
	    gridAddNodeWithGlobal(grid, 
				  x[cell_node], y[cell_node], z[cell_node], 
				  c2n[cell_node]-1 );
	  gridFreezeNode( grid, local_nodes[cell_node] );
	}
    }
  gridAddCell(grid, 
	      local_nodes[0], local_nodes[1], 
	      local_nodes[2], local_nodes[3] );
}

void FC_FUNC_(unravel_thaw,UNRAVEL_THAW)( int *nodeid, int *status )
{
  int local_node;
  *status = 0;

  local_node = gridGlobal2Local(grid, (*nodeid)-1 );

  if ( grid != gridThawNode( grid, local_node ) ) *status = 1;
}

void FC_FUNC_(unravel_it,UNRAVEL_IT)( int *status )
{
  int node, try;
  double volume, smallest_volume;

  *status = 1;
  for ( try = 0 ; try < 2 ; try ++ )
    {
      smallest_volume = DBL_MAX;
      for ( node = 0 ; node < gridMaxNode(grid) ; node++ )
	if ( !gridNodeFrozen(grid,node))
	  {
	    gridUntangleVolume( grid, node, 0, TRUE );
	    gridNodeVolume(grid, node, &volume );
	    smallest_volume = MIN( smallest_volume, volume );
	  }
      if ( smallest_volume > 1.0e-15 )
	{
	  *status = 0;
	  return;
	}
    }

}

void FC_FUNC_(unravel_xyz,UNRAVEL_XYZ)( int *nodeid, double *x, double *y, double *z, int *status )
{
  int local_node;
  double xyz[3];

  *status = 0;
  
  local_node = gridGlobal2Local(grid, (*nodeid)-1 );
  if ( grid != gridNodeXYZ( grid, local_node, xyz ) )  
    {
      *status = -1;
      return;
    }
  *x = xyz[0];
  *y = xyz[1];
  *z = xyz[2];
}

void FC_FUNC_(unravel_cleanup,UNRAVEL_CLEANUP)( int *status )
{

  *status = 0;
  
 gridFree(grid);
 grid = NULL;
}

