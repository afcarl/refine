
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_adapt.h"
#include "ref_edge.h"

#include "ref_malloc.h"
#include "ref_mpi.h"

#include "ref_cavity.h"
#include "ref_collapse.h"
#include "ref_smooth.h"
#include "ref_split.h"
#include "ref_swap.h"

#include "ref_matrix.h"
#include "ref_node.h"

#include "ref_gather.h"

REF_STATUS ref_adapt_create(REF_ADAPT *ref_adapt_ptr) {
  REF_ADAPT ref_adapt;

  ref_malloc(*ref_adapt_ptr, 1, REF_ADAPT_STRUCT);

  ref_adapt = *ref_adapt_ptr;

  ref_adapt->split_ratio = 1.5;
  ref_adapt->split_quality_absolute = 1.0e-3;
  ref_adapt->split_quality_relative = 0.6;

  ref_adapt->collapse_ratio = 0.6;
  ref_adapt->collapse_quality_absolute = 1.0e-3;
  ref_adapt->collapse_ratio_limit = 3.0;

  ref_adapt->smooth_min_quality = 1.0e-3;

  ref_adapt->collapse_per_pass = 1;

  ref_adapt->instrument = REF_FALSE;

  return REF_SUCCESS;
}

REF_STATUS ref_adapt_deep_copy(REF_ADAPT *ref_adapt_ptr, REF_ADAPT original) {
  REF_ADAPT ref_adapt;

  ref_malloc(*ref_adapt_ptr, 1, REF_ADAPT_STRUCT);

  ref_adapt = *ref_adapt_ptr;

  ref_adapt->split_ratio = original->split_ratio;
  ref_adapt->split_quality_absolute = original->split_quality_absolute;
  ref_adapt->split_quality_relative = original->split_quality_relative;

  ref_adapt->collapse_ratio = original->collapse_ratio;
  ref_adapt->collapse_quality_absolute = original->collapse_quality_absolute;
  ref_adapt->collapse_ratio_limit = original->collapse_ratio_limit;

  ref_adapt->smooth_min_quality = original->smooth_min_quality;

  ref_adapt->collapse_per_pass = original->collapse_per_pass;

  ref_adapt->instrument = original->instrument;

  return REF_SUCCESS;
}

REF_STATUS ref_adapt_free(REF_ADAPT ref_adapt) {
  ref_free(ref_adapt);

  return REF_SUCCESS;
}

REF_STATUS ref_adapt_parameter(REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_ADAPT ref_adapt = ref_grid->adapt;
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL det, complexity;
  REF_DBL quality, min_quality;
  REF_DBL volume, min_volume, max_volume;
  REF_BOOL active_twod;
  REF_DBL target;
  REF_INT cell_node;
  REF_INT node, nnode;
  REF_DBL nodes_per_complexity;
  REF_INT degree, max_degree;

  if (ref_grid_twod(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
  } else {
    ref_cell = ref_grid_tet(ref_grid);
  }

  min_quality = 1.0;
  min_volume = 1.0e100;
  max_volume = -1.0e100;
  complexity = 0.0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_node_node_twod(ref_grid_node(ref_grid), nodes[0], &active_twod),
          "active twod tri");
      if (!active_twod) continue;
      RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
      RSS(ref_node_tri_area(ref_grid_node(ref_grid), nodes, &volume), "vol");
    } else {
      RSS(ref_node_tet_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
      RSS(ref_node_tet_vol(ref_grid_node(ref_grid), nodes, &volume), "vol");
    }
    min_quality = MIN(min_quality, quality);
    min_volume = MIN(min_volume, volume);
    max_volume = MAX(max_volume, volume);

    for (cell_node = 0; cell_node < ref_cell_node_per(ref_cell); cell_node++) {
      if (ref_node_owned(ref_node, nodes[cell_node])) {
        RSS(ref_matrix_det_m(ref_node_metric_ptr(ref_node, nodes[cell_node]),
                             &det),
            "det");
        complexity +=
            sqrt(det) * volume / ((REF_DBL)ref_cell_node_per(ref_cell));
      }
    }
  }
  quality = min_quality;
  RSS(ref_mpi_min(ref_mpi, &quality, &min_quality, REF_DBL_TYPE), "min");
  RSS(ref_mpi_bcast(ref_mpi, &quality, 1, REF_DBL_TYPE), "min");
  volume = min_volume;
  RSS(ref_mpi_min(ref_mpi, &volume, &min_volume, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &min_volume, 1, REF_DBL_TYPE), "min");
  volume = max_volume;
  RSS(ref_mpi_max(ref_mpi, &volume, &max_volume, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_volume, 1, REF_DBL_TYPE), "min");

  RSS(ref_mpi_allsum(ref_mpi, &complexity, 1, REF_DBL_TYPE), "dbl sum");

  nnode = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      nnode++;
    }
  }
  RSS(ref_mpi_allsum(ref_mpi, &nnode, 1, REF_INT_TYPE), "int sum");
  if (ref_grid_twod(ref_grid)) nnode = nnode / 2;

  nodes_per_complexity = (REF_DBL)nnode / complexity;

  max_degree = 0;
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_adj_degree(ref_cell_adj(ref_cell), node, &degree), "cell degree");
    max_degree = MAX(max_degree, degree);
  }
  degree = max_degree;
  RSS(ref_mpi_max(ref_mpi, &degree, &max_degree, REF_INT_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_degree, 1, REF_INT_TYPE), "min");

  target = MAX(MIN(0.1, min_quality), 1.0e-3);

  if (ref_grid_once(ref_grid)) {
    printf("quality floor %6.4f max cell degree %d\n", target, max_degree);
    printf("nnode %10d complexity %12.1f ratio %5.2f\nvolume range %e %e\n",
           nnode, complexity, nodes_per_complexity, max_volume, min_volume);
  }
  ref_adapt->collapse_quality_absolute = target;
  ref_adapt->smooth_min_quality = target;

  return REF_SUCCESS;
}

REF_STATUS ref_adapt_pass(REF_GRID ref_grid) {
  if (ref_grid_twod(ref_grid)) {
    RSS(ref_adapt_twod_pass(ref_grid), "pass");
  } else {
    RSS(ref_adapt_threed_pass(ref_grid), "pass");
  }
  return REF_SUCCESS;
}

REF_STATUS ref_adapt_threed_pass(REF_GRID ref_grid) {
  REF_INT ngeom;
  REF_INT pass;

  RSS(ref_gather_ngeom(ref_grid_node(ref_grid), ref_grid_geom(ref_grid),
                       REF_GEOM_FACE, &ngeom),
      "count ngeom");
  ref_gather_blocking_frame(ref_grid, "threed pass");
  if (ngeom > 0) RSS(ref_geom_verify_topo(ref_grid), "adapt preflight check");
  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt start");

  for (pass = 0; pass < ref_grid_adapt(ref_grid, collapse_per_pass); pass++) {
    RSS(ref_collapse_pass(ref_grid), "col pass");
    ref_gather_blocking_frame(ref_grid, "collapse");
    if (ngeom > 0)
      RSS(ref_geom_verify_topo(ref_grid), "collapse geom typo check");
    if (ref_grid_adapt(ref_grid, instrument))
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt col");
  }

  RSS(ref_split_pass(ref_grid), "split pass");
  ref_gather_blocking_frame(ref_grid, "split");
  if (ngeom > 0) RSS(ref_geom_verify_topo(ref_grid), "split geom typo check");
  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt spl");

  RSS(ref_smooth_threed_pass(ref_grid), "smooth pass");
  ref_gather_blocking_frame(ref_grid, "smooth");
  if (ngeom > 0) RSS(ref_geom_verify_topo(ref_grid), "smooth geom typo check");
  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt mov");

  return REF_SUCCESS;
}

REF_STATUS ref_adapt_twod_pass(REF_GRID ref_grid) {
  ref_gather_blocking_frame(ref_grid, "twod pass");
  RSS(ref_collapse_twod_pass(ref_grid), "col pass");
  ref_gather_blocking_frame(ref_grid, "collapse");
  RSS(ref_split_twod_pass(ref_grid), "split pass");
  ref_gather_blocking_frame(ref_grid, "split");
  RSS(ref_smooth_twod_pass(ref_grid), "smooth pass");
  ref_gather_blocking_frame(ref_grid, "smooth");

  return REF_SUCCESS;
}
