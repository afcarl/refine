
#ifndef REF_IMPORT_H
#define REF_IMPORT_H

#include "ref_grid.h"
#include "ref_node.h"
#include "ref_cell.h"
#include "ref_metric.h"

BEGIN_C_DECLORATION

REF_STATUS ref_import_ugrid( char *filename, REF_GRID *ref_grid );
REF_STATUS ref_import_fgrid( char *filename, REF_GRID *ref_grid );

END_C_DECLORATION

#endif /* REF_IMPORT_H */
