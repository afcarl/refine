#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_import.h"
#include "ref_export.h"
#include "ref_test.h"

#include "ref_adj.h"
#include "ref_grid.h"
#include "ref_node.h"
#include "ref_cell.h"
#include "ref_metric.h"

int main( int argc, char *argv[] )
{
  REF_GRID ref_grid;

  if (argc<3) 
    {
      printf("usage: %s filename.ugrid filename.tec\n",argv[0]);
      return 0;
    }

  TSS(ref_import_ugrid( argv[1], &ref_grid ),"from ugrid");

  TSS(ref_export_tec( ref_grid, argv[2] ),"tec export" );

  TSS(ref_grid_free(ref_grid),"free");

  return 0;
}
