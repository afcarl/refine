
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
#include "gridmetric.h"
#include "gridinsert.h"
#include "gridswap.h"
#include "gridmpi.h"

Grid *gridIdentityNodeGlobal(Grid *grid, int offset )
{
  int node;
  for (node = 0; node < gridNNode(grid) ; node++ )
    if (grid != gridSetNodeGlobal(grid,node,node+offset)) return NULL;
  return grid;
}

Grid *gridIdentityCellGlobal(Grid *grid, int offset )
{
  int cell;
  for (cell = 0; cell < gridNCell(grid) ; cell++ )
    if (grid != gridSetCellGlobal(grid,cell,cell+offset)) return NULL;
  return grid;
}

Grid *gridSetAllLocal(Grid *grid )
{
  int node;
  for (node = 0; node < gridNNode(grid) ; node++ )
    if (grid != gridSetNodePart(grid,node,gridPartId(grid))) return NULL;
  return grid;
}

Grid *gridSetGhost(Grid *grid, int node )
{
  return gridSetNodePart(grid,node,EMPTY);
}

int gridParallelEdgeSplit(Grid *grid, Queue *queue, int node0, int node1 )
{
  double xyz0[3], xyz1[3];
  double newX, newY, newZ;
  int newnode;
  bool gemLocal;

  if ( gridNodeGhost(grid,node0) && gridNodeGhost(grid,node1) ) return EMPTY;

  gridMakeGem(grid, node0, node1 );
  gemLocal = gridGemIsAllLocal(grid);
  if ( NULL == queue && !gemLocal) return EMPTY;
  if ( NULL != queue && gemLocal) return EMPTY;

  if (grid != gridNodeXYZ(grid,node0,xyz0)) return EMPTY;
  if (grid != gridNodeXYZ(grid,node1,xyz1)) return EMPTY;

  newX = ( xyz0[0] + xyz1[0] ) * 0.5;
  newY = ( xyz0[1] + xyz1[1] ) * 0.5;
  newZ = ( xyz0[2] + xyz1[2] ) * 0.5;

  if (NULL != queue) queueNewTransaction(queue);
  newnode = gridSplitEdgeAt( grid, queue, node0, node1, newX, newY, newZ );
  if (EMPTY == newnode) {
    printf("WARNING: %s: %d: gridSplitEdgeAt returned EMPTY.\n",
	   __FILE__,__LINE__);
    return EMPTY;
  }
  
  return newnode;
}

Grid *gridParallelEdgeSwap(Grid *grid, Queue *queue, int node0, int node1 )
{
  bool gemLocal;
  Grid *result;

  if ( gridNodeGhost(grid,node0) && gridNodeGhost(grid,node1) ) return NULL;

  if ( grid != gridMakeGem(grid, node0, node1 ) ) {
    printf("gridParallelEdgeSwap gem error\n");
    return NULL;
  }
  gemLocal = gridGemIsAllLocal(grid);
  if ( NULL == queue && !gemLocal) return NULL;
  if ( NULL != queue && gemLocal) return NULL;

  queueNewTransaction(queue);
  result = gridSwapEdge( grid, queue, node0, node1 );

  if (NULL != result) {
    if (0 == gridCellDegree(grid,node0)) gridRemoveNode(grid,node0);
    if (0 == gridCellDegree(grid,node1)) gridRemoveNode(grid,node1);
  }

  return result;
}

Grid *gridApplyQueue(Grid *grid, Queue *gq )
{
  int transaction;
  int removed, removedcell, removedface;
  int i, globalnodes[9], localnodes[4];
  int cell, face;
  int added, addedcell, addedface;
  double xyz[1000], uv[6];
  int dim, aux;
  Queue *lq;

  dim = 3 + 6 + gridNAux(grid);
  if (dim>250) printf( "ERROR: %s: %d: undersized static xyz.\n", 
		       __FILE__, __LINE__);

  lq = queueCreate( 1 ); /* only used for queuing local removed nodes */

  removedcell = 0;
  removedface = 0;
  addedcell = 0;
  addedface = 0;
  for (transaction=0;transaction<queueTransactions(gq);transaction++){
    for (removed=0;removed<queueRemovedCells(gq,transaction);removed++) {
      queueRemovedCellNodes( gq, removedcell, globalnodes );
      removedcell++;
      for(i=0;i<4;i++)localnodes[i]=gridGlobal2Local(grid,globalnodes[i]);
      cell = gridFindCell(grid,localnodes);
      if (grid == gridRemoveCellWithOutGlobal(grid,cell)) 
	queueRemoveCell(lq,localnodes);
    }
    for (removed=0;removed<queueRemovedFaces(gq,transaction);removed++) {
      queueRemovedFaceNodes( gq, removedface, globalnodes );
      removedface++;
      for(i=0;i<3;i++)localnodes[i]=gridGlobal2Local(grid,globalnodes[i]);
      face = gridFindFace(grid,localnodes[0],localnodes[1],localnodes[2]);
      gridRemoveFace(grid,face);
    }

    for(added=0;added<queueAddedCells(gq,transaction);added++) {
      queueAddedCellNodes( gq, addedcell, globalnodes );
      queueAddedCellXYZs( gq, addedcell, xyz );
      addedcell++;
      for(i=0;i<4;i++)localnodes[i]=gridGlobal2Local(grid,globalnodes[i]);
      if ( gridNodeLocal(grid,localnodes[0]) ||
	   gridNodeLocal(grid,localnodes[1]) ||
	   gridNodeLocal(grid,localnodes[2]) ||
	   gridNodeLocal(grid,localnodes[3]) ) {
	for(i=0;i<4;i++) {
	  if ( EMPTY == localnodes[i] ) {
	    localnodes[i]=gridAddNode( grid,
				       xyz[0+dim*i],xyz[1+dim*i],xyz[2+dim*i]);
	    gridSetMap(grid,localnodes[i],
		       xyz[3+dim*i],xyz[4+dim*i],xyz[5+dim*i],
		       xyz[6+dim*i],xyz[7+dim*i],xyz[8+dim*i]);
	    for ( aux = 0 ; aux < gridNAux(grid) ; aux++ )     
	      gridSetAux(grid, localnodes[i], aux, xyz[aux+9+dim*i]);
	    gridSetNodeGlobal(grid, localnodes[i], globalnodes[i]);
	    gridSetNodePart(grid, localnodes[i], globalnodes[5+i]);
	  }
	}
	cell = gridAddCellWithGlobal(grid,
				     localnodes[0],localnodes[1],
				     localnodes[2],localnodes[3],
				     globalnodes[4]);
      }
    }
    for(added=0;added<queueAddedFaces(gq,transaction);added++) {
      queueAddedFaceNodes( gq, addedface, globalnodes );
      queueAddedFaceUVs( gq, addedface, uv );
      addedface++;
      for(i=0;i<3;i++)localnodes[i]=gridGlobal2Local(grid,globalnodes[i]);
      localnodes[3] = globalnodes[3];
      if ( gridNodeLocal(grid,localnodes[0]) ||
	   gridNodeLocal(grid,localnodes[1]) ||
	   gridNodeLocal(grid,localnodes[2]) ) {
	face = gridAddFaceUV(grid,
			     localnodes[0],uv[0],uv[1],
			     localnodes[1],uv[2],uv[3],
			     localnodes[2],uv[4],uv[5],
			     globalnodes[3]);
      }
    }
    queueNewTransaction(lq);
  }
  for (removed=0;removed<queueTotalRemovedCells(lq);removed++) {
    queueRemovedCellNodes( lq, removed, localnodes );
    for (i=0;i<4;i++) {
      if ( 0 == gridCellDegree(grid,localnodes[i]) ) {
	gridRemoveNode(grid,localnodes[i]);
      }
    }
  }
  
  printf( " %6d queue applied                           nnode %8d AR%14.10f\n",
	  gridPartId(grid),gridNNode(grid),gridMinAR(grid) );
  fflush(stdout);

  queueFree(lq);
  return grid;
}

Grid *gridParallelAdaptWithOutCAD(Grid *grid, Queue *queue,
				  double minLength, double maxLength )
{
  int n0, n1, adaptnode, origNNode, newnode;
  int nnodeAdd;
  double ratio;

  origNNode = gridNNode(grid);
  adaptnode =0;
  nnodeAdd = 0;

  for ( n0=0; adaptnode<origNNode; n0++ ) { 
    adaptnode++;
    if ( gridValidNode( grid, n0) && 
	 !gridNodeFrozen( grid, n0 ) && 
	 gridNodeLocal( grid, n0 ) ) {
      if ( NULL == gridLargestRatioEdge( grid, n0, &n1, &ratio) ) return NULL;
      if ( !gridNodeFrozen( grid, n1 ) && ratio > maxLength ) {
	newnode = gridParallelEdgeSplit(grid, queue, n0, n1);
	if ( newnode != EMPTY ){
	  nnodeAdd++;
	}
      }
    }else{
      adaptnode++;
    }
  }
  if ( NULL == queue ) {
    printf("local added%9d nnode%9d AR%14.10f\n",
	   nnodeAdd,gridNNode(grid),gridMinAR(grid));
  } else {
    printf("ghost added%9d nnode%9d AR%14.10f\n",
	   nnodeAdd,gridNNode(grid),gridMinAR(grid));
  }
  return grid;
}

Grid *gridParallelSwap(Grid *grid, Queue *queue )
{
  int cell, maxcell;
  int nodes[4];

  maxcell = gridMaxCell(grid);

  for (cell=0;cell<maxcell;cell++){
    if ( grid == gridCell( grid, cell, nodes) && gridAR(grid, nodes) < 0.5) {
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[1] ) )
	   continue;
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[2] ) )
	   continue;
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[3] ) )
	   continue;
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[1], nodes[2] ) )
	   continue;
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[1], nodes[3] ) )
	   continue;
      if ( grid == gridParallelEdgeSwap(grid, queue, nodes[2], nodes[3] ) )
	   continue;
    }
  }

  if ( NULL == queue ) {
    printf("                                    AR%14.10f\n",
	   gridMinAR(grid));
  } else {
    printf("                                    AR%14.10f\n",
	   gridMinAR(grid));
  }
  return grid;
}
