/*
 *  gameoflife.c : a simple cellular automata that implements a "game of life"
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"

//#define pointeradd(a, b) (int*)((int)(a)+(b))

/* 
 * Automata rules for cells that survive or die
 */
int newValue ( int* upper, int* grid, int index, int value, int* lower )
{	int m=index; 
	int count = upper[(m+15)%16] + upper[m] + upper[(m+1)%16] + lower[(m+15)%16] + lower[m] + lower[(m+1)%16] + grid[(m+15)%16] + grid[(m+1)%16];
	if (count==3 && value==0){
		return 1;
	}
	if ((count!=2 && count!=3) && value==1){
		return 0;
	}
        return value;
}

/*
 * gameoflife: S23/B3
 */
int main ( int argc, char** argv )
{
  /* Iteration variables */
  int i,j,k;

  /* Simulation variables */
  /* Local data */
  int num_rows;    //in local array
  int offset;     //local-> global
  int iterations = 64;


  /* Global constant data */
  const int dimension = 16;     // assume a square grid
  int global_grid[ 256 ] = { 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


  /* MPI Standard variable */
  int num_procs;
  int ID;

  /* Messaging variables */
  MPI_Status stat;
  int prev;
  int next; 

  /* MPI Setup */
  if ( MPI_Init(&argc, &argv) != MPI_SUCCESS )
  {
    printf ( "MPI_Init error\n" );
  }

  MPI_Comm_size ( MPI_COMM_WORLD, &num_procs );
  MPI_Comm_rank ( MPI_COMM_WORLD, &ID );

  /* Initialize process specific state */
  next = ID == num_procs-1 ? 0 : ID+1;
  prev = ID == 0 ? num_procs-1 : ID-1;   

  /* Setup the game of life */
  assert ( dimension % num_procs == 0 );  
  num_rows = dimension / num_procs;
  offset = ID * num_rows * dimension;

  int upper[(num_rows+2)*dimension]; //make a new block that contains all data needed for the local process
  int* grid = upper+dimension;
  int* lower = grid + num_rows*dimension;
  int tmp_grid[num_rows*dimension];

  for ( i=0; i < num_rows*dimension; i++ )
  {
	grid[i] = global_grid[ offset + i];
  }
 
  if (ID==0)
  {     printf ( "Initial grid state:\n");
        for ( j=0; j < dimension; j++ )
        {
            for  ( k=0; k<15; k++)
	    {	
	        printf ( "%d, ", global_grid[dimension*j+k] );
	    }
	    printf ( "%d\n", global_grid[dimension*j+15] );
        }
        printf ( "\n");
  }
  for ( i =1; i <= iterations; i++ )
  {  
    /* Send and receive point to point messages */
    if ( ID % 2 == 0 )
    {
      MPI_Send ( grid, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD ); 
      MPI_Send ( grid+dimension*(num_rows-1), dimension, MPI_INT, next, 2, MPI_COMM_WORLD ); 
  
      MPI_Recv ( lower, dimension, MPI_INT, next, 2, MPI_COMM_WORLD, &stat );
      MPI_Recv ( upper, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD, &stat );
      //printf ( "%d:%d received %d from %d and %d from %d\n", ID, i, lower, prev, upper, next );
    }
    else 
    { 
      MPI_Recv ( lower, dimension, MPI_INT, next, 2, MPI_COMM_WORLD, &stat );
      MPI_Recv ( upper, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD, &stat );
      //printf ( "%d:%d received %d from %d and %d from %d\n", ID, i, lower, prev, upper, next );
      MPI_Send ( grid, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD ); 
      MPI_Send ( grid+dimension*(num_rows-1), dimension, MPI_INT, next, 2, MPI_COMM_WORLD ); 
    }


    /* update the values */
    
    for ( j=0; j < num_rows; j++ )
    {	
	for  ( k=0; k<dimension; k++)
	{		
	    tmp_grid[dimension*j+k] = newValue ( grid+dimension*(j-1), grid+dimension*j, k, grid[dimension*j+k], grid+dimension*(j+1) ); 
	}
    }
    

    /* Copy over the old grid.  What else could we do here? */
    for ( j=0; j < num_rows*dimension; j++ )
    {
	grid[j] = tmp_grid[j];
    }

    MPI_Gather(grid, dimension*num_rows, MPI_INT, global_grid, dimension*num_rows, MPI_INT, 0, MPI_COMM_WORLD );


    /* Output the update grid state */
    if (ID ==0)
    {   printf("Iteration: %d, ID: %d\n", i, ID);
        for ( j=0; j < dimension; j++ )
        {
            for  ( k=0; k<15; k++)
	    {	
	        printf ( "%d, ", global_grid[dimension*j+k] );
	    }
	    printf ( "%d\n", global_grid[dimension*j+15] );
        }
        printf ( "\n");
    }
    
    
  }
  MPI_Finalize();

}
