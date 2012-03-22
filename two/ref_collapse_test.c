#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_grid.h"
#include "ref_cell.h"
#include "ref_edge.h"
#include "ref_node.h"
#include "ref_list.h"
#include "ref_adj.h"
#include "ref_sort.h"
#include "ref_dict.h"
#include "ref_matrix.h"

#include "ref_mpi.h"

#include "ref_collapse.h"

#include "ref_fixture.h"
#include "ref_export.h"

int main( void )
{

  { /* collapse tet in to triangle */
    REF_GRID ref_grid;
    REF_INT node0, node1;

    RSS(ref_fixture_tet_grid(&ref_grid),"set up");
    node0 = 0; node1 = 3;

    RSS(ref_collapse_edge(ref_grid,node0,node1),"collapse");

    REIS(0, ref_cell_n(ref_grid_tet(ref_grid)),"tet");
    REIS(1, ref_cell_n(ref_grid_tri(ref_grid)),"tri");

    RSS( ref_grid_free( ref_grid ), "free grid");
  }

  { /* collapse tet in to nothing */
    REF_GRID ref_grid;
    REF_INT node0, node1;

    RSS(ref_fixture_tet_grid(&ref_grid),"set up");
    node0 = 0; node1 = 1;

    RSS(ref_collapse_edge(ref_grid,node0,node1),"collapse");

    REIS(0, ref_cell_n(ref_grid_tet(ref_grid)),"tet");
    REIS(0, ref_cell_n(ref_grid_tri(ref_grid)),"tri");

    RSS( ref_grid_free( ref_grid ), "free grid");
  }
  return 0;
}