
#include <stdlib.h>
#include <stdio.h>

#include "ref_inflate.h"

#include "ref_cell.h"
#include "ref_malloc.h"
#include "ref_math.h"

#include "ref_export.h"

REF_STATUS ref_inflate_pri_min_dot( REF_NODE ref_node, 
				    REF_INT *nodes,  
				    REF_DBL *min_dot )
{
  REF_INT tri_nodes[3];
  REF_DBL top_normal[3];
  REF_DBL bot_normal[3];
  REF_DBL edge[3];
  REF_INT node, i;

  tri_nodes[0]= nodes[0];
  tri_nodes[1]= nodes[1];
  tri_nodes[2]= nodes[2];
  RSS( ref_node_tri_normal( ref_node, tri_nodes, bot_normal ), "bot"); 
  RSS( ref_math_normalize( bot_normal ), "norm bot");

  tri_nodes[0]= nodes[3];
  tri_nodes[1]= nodes[4];
  tri_nodes[2]= nodes[5];
  RSS( ref_node_tri_normal( ref_node, tri_nodes, top_normal ), "top"); 
  RSS( ref_math_normalize( top_normal ), "norm top");

  *min_dot = 1.0;
  for ( node=0;node<3;node++)
    {
      for ( i = 0; i < 3 ; i++ )
	edge[i] = ref_node_xyz(ref_node,i,nodes[node+3]) 
	  - ref_node_xyz(ref_node,i,nodes[node]); 
      RSS( ref_math_normalize( edge ), "norm edge0");
      *min_dot = MIN(*min_dot,ref_math_dot(edge,bot_normal));
      *min_dot = MIN(*min_dot,ref_math_dot(edge,top_normal));
    }

  return REF_SUCCESS;
}

REF_STATUS ref_inflate_face( REF_GRID ref_grid, 
			     REF_DICT faceids, 
			     REF_DBL *origin, 
			     REF_DBL thickness, REF_DBL xshift )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL qua = ref_grid_qua(ref_grid);
  REF_CELL pri = ref_grid_pri(ref_grid);
  REF_INT cell, tri_side, node0, node1;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ntri, tris[2], nquad, quads[2];
  REF_INT tri_node;
  REF_INT *o2n;
  REF_INT global, new_node;
  REF_INT new_cell;
  REF_DBL min_dot;

  REF_DBL normal[3], dot;
  REF_INT ref_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, ref;

  REF_DBL *tmin, *tmax;
  REF_INT *imin, *imax;
  REF_DBL *face_normal;
  REF_INT i;
  REF_DBL theta;

  REF_BOOL problem_detected = REF_FALSE;

  REF_BOOL debug = REF_TRUE;

  ref_malloc_init( o2n, ref_node_max(ref_node), 
		   REF_INT, REF_EMPTY );

  ref_malloc( face_normal, 3*ref_dict_n(faceids), REF_DBL );

  /* determine each faceids normal */

  ref_malloc_init( tmin, ref_dict_n(faceids), REF_DBL,  2.0*ref_math_pi );
  ref_malloc_init( tmax, ref_dict_n(faceids), REF_DBL, -2.0*ref_math_pi);
  ref_malloc_init( imin, ref_dict_n(faceids), REF_INT,  REF_EMPTY);
  ref_malloc_init( imax, ref_dict_n(faceids), REF_INT,  REF_EMPTY);

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      for(tri_node=0;tri_node<3;tri_node++)
	{
	  RSS(ref_dict_location(faceids, nodes[3], &i),"key loc");
	  node0 = nodes[tri_node];
	  theta = atan2( ref_node_xyz(ref_node,1,node0)-origin[1],
			 ref_node_xyz(ref_node,2,node0)-origin[2]);
	  if ( tmin[i] > theta )
	    {
	      tmin[i] = theta;
	      imin[i] = node0;
	    }
	  if ( tmax[i] < theta )
	    {
	      tmax[i] = theta;
	      imax[i] = node0;
	    }
	}

  if (debug)
    ref_dict_inspect( faceids );

  each_ref_dict_key_index( faceids, i )
    {
      RUS( REF_EMPTY, imin[i],"imin");
      RUS( REF_EMPTY, imax[i],"imax");
      face_normal[0+3*i] = 0.0;
      face_normal[1+3*i] =  ( ref_node_xyz(ref_node,2,imax[i]) -
			      ref_node_xyz(ref_node,2,imin[i]) );
      face_normal[2+3*i] = -( ref_node_xyz(ref_node,1,imax[i]) -
			      ref_node_xyz(ref_node,1,imin[i]) );
      if (debug)
	printf("faceid[%d]=%d t=(%f,%f)\nn=(%f,%f,%f)\n",
	       i, ref_dict_key( faceids, i ),
	       tmin[i],tmax[i],
	       face_normal[0+3*i], face_normal[1+3*i], face_normal[2+3*i]);
      RSS( ref_math_normalize( &(face_normal[3*i]) ), "make face norm" );
      if (debug)
	printf("n=(%f,%f,%f)\n",
	       face_normal[0+3*i], face_normal[1+3*i], face_normal[2+3*i]);
   }

  ref_free( tmax );
  ref_free( tmin );
  ref_free( imax );
  ref_free( imin );

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      for(tri_node=0;tri_node<3;tri_node++)
	{
	  node0 = nodes[tri_node];
	  if ( REF_EMPTY == o2n[node0] )
	    {
	      RSS( ref_node_next_global( ref_node, &global ), "global" );
	      RSS( ref_node_add( ref_node, global, &new_node ), "add node" );
	      o2n[node0] = new_node;

	      RAS( ref_node_valid(ref_node,node0),"inlvalid tri node");
	      normal[0]=0.0;
	      normal[1]=ref_node_xyz(ref_node,1,node0)-origin[1];
	      normal[2]=ref_node_xyz(ref_node,2,node0)-origin[2];
	      RSS( ref_math_normalize( normal ), "make norm" );
	      each_ref_cell_having_node( tri, node0, item, ref )
		{
		  ref_cell_nodes( tri, ref, ref_nodes );
		  if ( !ref_dict_has_key( faceids, ref_nodes[3] ) )
		    continue;
		  RSS(ref_dict_location(faceids, ref_nodes[3], &i),"key loc");
		  dot = -ref_math_dot(normal, &(face_normal[3*i]));
		  if ( dot < 0.70 || dot > 1.01 ) 
		    {
		      /* printf("out-of-range dot %.15f\n",dot); */
		      problem_detected = REF_TRUE;
		    }
		  normal[1] /= dot;
		  normal[2] /= dot;
		}
	      ref_node_xyz(ref_node,0,new_node) = 
		xshift + ref_node_xyz(ref_node,0,node0);
	      ref_node_xyz(ref_node,1,new_node) = 
		thickness*normal[1] + ref_node_xyz(ref_node,1,node0);
	      ref_node_xyz(ref_node,2,new_node) = 
		thickness*normal[2] + ref_node_xyz(ref_node,2,node0);
	    }
	}

  ref_free( face_normal );

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	for(tri_side=0;tri_side<3;tri_side++)
	  {
	    node0 = ref_cell_e2n(tri,0,tri_side,cell);
	    node1 = ref_cell_e2n(tri,1,tri_side,cell);
	    RSS( ref_cell_list_with2( tri, 
				      node0, node1,
				      2, &ntri,
				      tris ),"bad tri count");
	    if ( 1 == ntri ) 
	      {
		RSS( ref_cell_list_with2( qua, 
					  node0, node1,
					  2, &nquad,
					  quads ),"bad quad count");
		if ( 1 != nquad ) THROW("tri without quad");
		new_nodes[4] = ref_cell_c2n(qua,4,quads[0]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	    if (  ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[0]) ) &&
		 !ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[1]) ) )
	      {
		new_nodes[4] = ref_cell_c2n(tri,3,tris[1]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	    if ( !ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[0]) ) &&
		  ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[1]) ) )
	      {
		new_nodes[4] = ref_cell_c2n(tri,3,tris[0]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	  }
      }

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	new_nodes[0] = nodes[0];
	new_nodes[1] = nodes[2];
	new_nodes[2] = nodes[1];
	new_nodes[3] = o2n[nodes[0]];
	new_nodes[4] = o2n[nodes[2]];
	new_nodes[5] = o2n[nodes[1]];
	
	RSS( ref_inflate_pri_min_dot( ref_node, new_nodes, &min_dot ), "md");
	if ( min_dot <= 0.0 ) 
	  {
	    /* printf("min_dot %f\n",min_dot); */
	    problem_detected = REF_TRUE;
	  }
	
	RSS( ref_cell_add( pri, new_nodes, &new_cell ), "pri");
      }

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	nodes[0] = o2n[nodes[0]];
	nodes[1] = o2n[nodes[1]];
	nodes[2] = o2n[nodes[2]];
	RSS( ref_cell_replace_whole( tri, cell, nodes ), "repl");
      }

  ref_free( o2n );

  if ( problem_detected )
    {
      RSS( ref_export_tec_surf( ref_grid, "ref_inflate_problem.tec" ), "tec" );
      THROW("problem detected, examine ref_inflate_problem.tec");
    }

  return REF_SUCCESS;
}

REF_STATUS ref_inflate_radially( REF_GRID ref_grid, 
				 REF_DICT faceids, 
				 REF_DBL *origin, 
				 REF_DBL thickness, 
				 REF_DBL mach_angle_rad,
				 REF_DBL alpha_rad )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL qua = ref_grid_qua(ref_grid);
  REF_CELL pri = ref_grid_pri(ref_grid);
  REF_INT cell, tri_side, node0, node1;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ntri, tris[2], nquad, quads[2];
  REF_INT tri_node;
  REF_INT *o2n;
  REF_INT global, new_node;
  REF_INT new_cell;
  REF_DBL min_dot;
  REF_DBL phi_rad, alpha_weighting, xshift;

  REF_DBL normal[3];

  REF_BOOL problem_detected = REF_FALSE;

  ref_malloc_init( o2n, ref_node_max(ref_node), 
		   REF_INT, REF_EMPTY );

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      for(tri_node=0;tri_node<3;tri_node++)
	{
	  node0 = nodes[tri_node];
	  if ( REF_EMPTY == o2n[node0] )
	    {
	      RSS( ref_node_next_global( ref_node, &global ), "global" );
	      RSS( ref_node_add( ref_node, global, &new_node ), "add node" );
	      o2n[node0] = new_node;

	      RAS( ref_node_valid(ref_node,node0),"inlvalid tri node");
	      normal[0]=0.0;
	      normal[1]=ref_node_xyz(ref_node,1,node0)-origin[1];
	      normal[2]=ref_node_xyz(ref_node,2,node0)-origin[2];
	      RSS( ref_math_normalize( normal ), "make norm" );
	      phi_rad = atan2( normal[1], normal[2] );
	      alpha_weighting = cos(phi_rad);
	      xshift = thickness 
		/ tan(mach_angle_rad+alpha_weighting*alpha_rad);
	      ref_node_xyz(ref_node,0,new_node) = 
		xshift + ref_node_xyz(ref_node,0,node0);
	      ref_node_xyz(ref_node,1,new_node) = 
		thickness*normal[1] + ref_node_xyz(ref_node,1,node0);
	      ref_node_xyz(ref_node,2,new_node) = 
		thickness*normal[2] + ref_node_xyz(ref_node,2,node0);
	    }
	}

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	for(tri_side=0;tri_side<3;tri_side++)
	  {
	    node0 = ref_cell_e2n(tri,0,tri_side,cell);
	    node1 = ref_cell_e2n(tri,1,tri_side,cell);
	    RSS( ref_cell_list_with2( tri, 
				     node0, node1,
				     2, &ntri,
				     tris ),"bad tri count");
	    if ( 1 == ntri ) 
	      {
		RSS( ref_cell_list_with2( qua, 
					 node0, node1,
					 2, &nquad,
					 quads ),"bad quad count");
		if ( 1 != nquad ) THROW("tri without quad");
		new_nodes[4] = ref_cell_c2n(qua,4,quads[0]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	    if (  ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[0]) ) &&
		 !ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[1]) ) )
	      {
		new_nodes[4] = ref_cell_c2n(tri,3,tris[1]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	    if ( !ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[0]) ) &&
		  ref_dict_has_key( faceids, ref_cell_c2n(tri,3,tris[1]) ) )
	      {
		new_nodes[4] = ref_cell_c2n(tri,3,tris[0]);
		new_nodes[0] = node0;
		new_nodes[1] = node1;
		new_nodes[2] = o2n[node1];
		new_nodes[3] = o2n[node0];
		RSS( ref_cell_add( qua, new_nodes, &new_cell ), "qua tri1");
		continue;
	      }
	  }
      }

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	new_nodes[0] = nodes[0];
	new_nodes[1] = nodes[2];
	new_nodes[2] = nodes[1];
	new_nodes[3] = o2n[nodes[0]];
	new_nodes[4] = o2n[nodes[2]];
	new_nodes[5] = o2n[nodes[1]];
	
	RSS( ref_inflate_pri_min_dot( ref_node, new_nodes, &min_dot ), "md");
	if ( min_dot <= 0.0 ) 
	  {
	    printf("min_dot %f\n",min_dot);
	    problem_detected = REF_TRUE;
	  }
	
	RSS( ref_cell_add( pri, new_nodes, &new_cell ), "pri");
      }

  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	nodes[0] = o2n[nodes[0]];
	nodes[1] = o2n[nodes[1]];
	nodes[2] = o2n[nodes[2]];
	RSS( ref_cell_replace_whole( tri, cell, nodes ), "repl");
      }

  ref_free( o2n );

  if ( problem_detected )
    {
      RSS( ref_export_tec_surf( ref_grid, "ref_inflate_problem.tec" ), "tec" );
      THROW("problem detected, examine ref_inflate_problem.tec");
    }

  return REF_SUCCESS;
}

REF_STATUS ref_inflate_rate( REF_INT nlayers,
			     REF_DBL first_thickness,
			     REF_DBL total_thickness,
			     REF_DBL *rate )
{
  REF_DBL r, H, err, dHdr;
  REF_BOOL keep_going;
  REF_INT iters;
  r = 1.1;

  iters = 0;
  keep_going = REF_TRUE;
  while (keep_going)
    {
      iters++;
      if ( iters > 100) THROW("iteration count exceeded");

      RSS( ref_inflate_total_thickness(nlayers,first_thickness,r,&H),"total");
      err = H-total_thickness;

      RSS( ref_inflate_dthickness(nlayers,first_thickness,r,&dHdr),"total");

      /* printf(" r %e H %e err %e dHdr %e\n",r,H,err,dHdr); */

      r = r - err/dHdr;

      keep_going = ( ABS(err/total_thickness) > 1.0e-12 );
    }

  *rate = r;

  return REF_SUCCESS;
}

REF_STATUS ref_inflate_total_thickness( REF_INT nlayers,
					REF_DBL first_thickness,
					REF_DBL rate,
					REF_DBL *total_thickness )
{
  *total_thickness = first_thickness*(1.0-pow(rate,nlayers))/(1.0-rate);

  return REF_SUCCESS;
}

REF_STATUS ref_inflate_dthickness( REF_INT nlayers,
				   REF_DBL first_thickness,
				   REF_DBL rate,
				   REF_DBL *dHdr )
{
  REF_DBL dnum, dden;
  dnum = -((REF_DBL)nlayers)*pow(rate,nlayers-1);
  dden = -1.0;

  *dHdr = first_thickness * ( dnum*(1.0-rate) 
  			   - (1.0-pow(rate,nlayers))*dden )
    /(1.0-rate)/(1.0-rate) ;
  return REF_SUCCESS;
}

REF_STATUS ref_inflate_origin( REF_GRID ref_grid,
			       REF_DICT faceids,
			       REF_DBL *origin )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL z0, z1;
  REF_INT node;
  REF_BOOL first_time;

  first_time = REF_TRUE;
  z0=0;z1=0;
  each_ref_cell_valid_cell_with_nodes( tri, cell, nodes)
    if ( ref_dict_has_key( faceids, nodes[3] ) )
      {
	for (node=0;node<3;node++)
	  {
	    if ( !first_time )
	      {
		z0 = MIN( z0, ref_node_xyz(ref_node,2,nodes[node]) );
		z1 = MAX( z1, ref_node_xyz(ref_node,2,nodes[node]) );
	      }
	    else
	      {
		z0 = ref_node_xyz(ref_node,2,nodes[node]);
		z1 = ref_node_xyz(ref_node,2,nodes[node]);		
		first_time = REF_FALSE;
	      }
	  }
      }

  origin[0] = 0;
  origin[1] = 0;
  origin[2] = 0.5*(z0+z1);

  printf("the z range is %f %f and origin %f\n",z0,z1,origin[2]);

  return REF_SUCCESS;
}
