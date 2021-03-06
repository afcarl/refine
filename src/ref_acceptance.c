
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

#include "ref_math.h"

#include "ref_export.h"
#include "ref_import.h"

#include "ref_adj.h"
#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_grid.h"
#include "ref_list.h"
#include "ref_matrix.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_sort.h"

#include "ref_edge.h"

#include "ref_args.h"
#include "ref_fixture.h"
#include "ref_gather.h"
#include "ref_metric.h"

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;
  REF_GRID ref_grid;
  REF_NODE ref_node;
  REF_INT masabl_pos;
  REF_INT ugawg_pos;
  REF_INT polar2d_pos;

  RSS(ref_mpi_create(&ref_mpi), "create");

  RXS(ref_args_find(argc, argv, "-ugawg", &ugawg_pos), REF_NOT_FOUND, "arg");

  if (REF_EMPTY != ugawg_pos) {
    REF_BOOL metric_recognized = REF_FALSE;
    if (5 != argc) {
      printf("usage:\n");
      printf(
          "  %s -ugawg [linear,polar-1,polar-2] input.grid_format "
          "output.metric\n",
          argv[0]);
      return (1);
    }
    printf("%s reading\n", argv[3]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[3]), "in");

    printf("%s field type\n", argv[2]);
    if (strcmp(argv[2], "linear") == 0) {
      metric_recognized = REF_TRUE;
      printf(" -ugawg linear metric\n");
      RSS(ref_metric_olympic_node(ref_grid_node(ref_grid), 0.001), "lin");
    }
    if (strcmp(argv[2], "polar-1") == 0) {
      metric_recognized = REF_TRUE;
      printf(" -ugawg polar-1 metric\n");
      RSS(ref_metric_ugawg_node(ref_grid_node(ref_grid), 1), "p1");
    }
    if (strcmp(argv[2], "polar-2") == 0) {
      metric_recognized = REF_TRUE;
      printf(" -ugawg polar-2 metric\n");
      RSS(ref_metric_ugawg_node(ref_grid_node(ref_grid), 2), "p2");
    }
    if (strcmp(argv[2], "ring") == 0) {
      metric_recognized = REF_TRUE;
      printf(" -ugawg ring metric\n");
      RSS(ref_metric_ring_node(ref_grid_node(ref_grid)), "ring");
    }
    RAS(metric_recognized, "did not recognize metric field name");
    printf("%s metric exported\n", argv[4]);
    RSS(ref_gather_metric(ref_grid, argv[4]), "in");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "-polar2d", &polar2d_pos), REF_NOT_FOUND,
      "arg");

  if (REF_EMPTY != polar2d_pos) {
    if (4 != argc) {
      printf("usage:\n");
      printf("  %s -polar2d input.grid_format output.metric\n", argv[0]);
      return (1);
    }
    printf("%s reading\n", argv[3]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "in");

    RSS(ref_metric_polar2d_node(ref_grid_node(ref_grid)), "lin");
    printf("%s metric exported\n", argv[3]);
    RSS(ref_gather_metric(ref_grid, argv[3]), "in");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "-masabl", &masabl_pos), REF_NOT_FOUND, "arg");

  if (REF_EMPTY != masabl_pos) {
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[1]), "in");

    RSS(ref_metric_masabl_node(ref_grid_node(ref_grid)), "masabl");
    if (ref_grid_twod(ref_grid))
      RSS(ref_metric_twod_node(ref_grid_node(ref_grid)), "2d");

    RSS(ref_gather_metric(ref_grid, argv[2]), "in");
    return 0;
  }

  if (3 == argc) {
    switch (atoi(argv[1])) {
      case 1:
        RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
        break;
      case 2:
        RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi), "brick");
        break;
      default:
        THROW("case not recognized");
        break;
    }
    RSS(ref_export_by_extension(ref_grid, argv[2]), "out");

    return 0;
  }

  if (4 == argc) {
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[1]), "in");

    RSS(ref_metric_olympic_node(ref_grid_node(ref_grid), atof(argv[3])), "oly");
    if (ref_grid_twod(ref_grid))
      RSS(ref_metric_twod_node(ref_grid_node(ref_grid)), "2d");

    RSS(ref_gather_metric(ref_grid, argv[2]), "in");

    return 0;
  }

  if (5 == argc) {
    REF_DBL x, z, h, c, r, k;
    REF_INT node;

    c = atof(argv[3]);
    k = atof(argv[4]);

    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[1]), "in");
    ref_node = ref_grid_node(ref_grid);

    each_ref_node_valid_node(ref_node, node) {
      x = ref_node_xyz(ref_node, 0, node);
      z = ref_node_xyz(ref_node, 2, node);
      r = sqrt(x * x + z * z);
      r = MAX(r, 0.32 * c * c);
      h = c * pow(r, k);

      ref_node_metric(ref_node, 0, node) = 1.0 / (h * h);
      ref_node_metric(ref_node, 1, node) = 0.0;
      ref_node_metric(ref_node, 2, node) = 0.0;
      ref_node_metric(ref_node, 3, node) = 1.0;
      ref_node_metric(ref_node, 4, node) = 0.0;
      ref_node_metric(ref_node, 5, node) = 1.0 / (h * h);
    }

    if (ref_grid_twod(ref_grid))
      RSS(ref_metric_twod_node(ref_grid_node(ref_grid)), "2d");

    RSS(ref_gather_metric(ref_grid, argv[2]), "in");

    return 0;
  }

  if (5 < argc) {
    REF_DBL h0, r;
    REF_DBL z, hx, hz;
    REF_DBL x0, c, k, x, h;
    REF_DBL m0[6], m1[6];

    REF_INT i;
    REF_INT pos;
    REF_INT node;

    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[1]), "in");
    ref_node = ref_grid_node(ref_grid);

    pos = 3;
    while (pos < argc) {
      if (strcmp(argv[pos], "-e") == 0) {
        printf("-p\n");
        pos++;
        hx = atof(argv[pos]);
        printf(" hx = %e\n", hx);
        pos++;
        h0 = atof(argv[pos]);
        printf(" h0 = %e\n", h0);
        pos++;
        r = atof(argv[pos]);
        printf(" r = %e\n", r);
        pos++;
        printf(" hz = h0 * (1+r) ** (z/h0)\n");

        each_ref_node_valid_node(ref_node, node) {
          z = ref_node_xyz(ref_node, 2, node);
          hz = h0 * pow(1.0 + r, z / h0);

          ref_node_metric(ref_node, 0, node) = 1.0 / (hx * hx);
          ref_node_metric(ref_node, 1, node) = 0.0;
          ref_node_metric(ref_node, 2, node) = 0.0;
          ref_node_metric(ref_node, 3, node) = 1.0;
          ref_node_metric(ref_node, 4, node) = 0.0;
          ref_node_metric(ref_node, 5, node) = 1.0 / (hz * hz);
        }

      } else if (strcmp(argv[pos], "-r") == 0) {
        printf("-r\n");
        pos++;
        x0 = atof(argv[pos]);
        printf(" x0 = %e\n", x0);
        pos++;
        c = atof(argv[pos]);
        printf(" c = %e\n", c);
        pos++;
        k = atof(argv[pos]);
        printf(" k = %e\n", k);
        pos++;
        printf(" at x0, h = c * r ** k\n");

        each_ref_node_valid_node(ref_node, node) {
          x = ref_node_xyz(ref_node, 0, node);
          z = ref_node_xyz(ref_node, 2, node);
          r = sqrt((x - x0) * (x - x0) + z * z);
          r = MAX(r, 0.32 * c * c);
          h = c * pow(r, k);

          for (i = 0; i < 6; i++) m0[i] = ref_node_metric(ref_node, i, node);

          m1[0] = 1.0 / (h * h);
          m1[1] = 0.0;
          m1[2] = 0.0;
          m1[3] = 1.0;
          m1[4] = 0.0;
          m1[5] = 1.0 / (h * h);

          RSS(ref_matrix_intersect(m0, m1, ref_node_metric_ptr(ref_node, node)),
              "intersect");
        }

      } else if (strcmp(argv[pos], "-h") == 0) {
        printf(" usage\n");
        return (0);
      } else {
        fprintf(stderr, "Argument \"%s\" Ignored\n", argv[pos]);
        pos++;
      }
    }

    if (ref_grid_twod(ref_grid))
      RSS(ref_metric_twod_node(ref_grid_node(ref_grid)), "2d");

    RSS(ref_gather_metric(ref_grid, argv[2]), "in");

    return 0;
  }

  return 0;
}
