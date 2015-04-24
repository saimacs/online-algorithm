#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "my402list.h"
#include "cs402.h"
#include <mpi.h>
#include <math.h>
#include <unistd.h>

typedef struct cluster_struct{
long double *centroid;
long double maxradius;
int number;
int centroidSet;
My402List datapoints;
}Cluster;

typedef struct calc_radius{
long double radius_value;
int proc_rank;
}sendstruct;

typedef struct centroid_data{
long double *centroid;
int rank;
}centroidData;

int main(int argc, char *argv[]){
	if(argc<5)
	{
		fprintf(stderr, "Error:@Line:%d:: Insufficient parameters to execute the program, online <alpha> <beta> <number of clusters> <dimensions>",__LINE__);
		exit(1);
	}
	int alpha=strtol(argv[1],NULL,10);
	int beta=strtol(argv[2],NULL,10);
	int nclusters=strtol(argv[3],NULL,10);
    int dimension=strtol(argv[4],NULL,10);
	int rank,nprocs;	
	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Cluster* newCluster=(Cluster *)malloc(sizeof(Cluster)),*extraCluster;
    newCluster->number=rank;
    memset(&newCluster->datapoints,0,sizeof(My402List));
	My402ListInit(&newCluster->datapoints);
    newCluster->centroid=(long double*)calloc(sizeof(long double)*dimension,sizeof(long double));
    newCluster->maxradius=0;
	newCluster->centroidSet=0;
    int onlinePhase=1;
    int mergePhase=0;
	long double global_radius=-1, distance=0;
	int newpoints_arrived=0;
	sendstruct in, out;
	MPI_Status status;
	My402List centroids_atroot;
	memset(&centroids_atroot,0,sizeof(My402List));
	My402ListInit(&centroids_atroot);
	FILE* fp=fopen("onlinedata.txt","r");
	long double centroidArray[dimension],extraClusterCentroid[dimension];

	while(onlinePhase){
		long double newData[dimension];
	    if(rank==0)
		{
			char *temp_token,buf[1000];
			//printf("...checking\n");
			if(fgets(buf,1000,fp)!=NULL)
			{
				//printf("no new line\n");
				int i=1;
				temp_token=strtok(buf,",");
				newData[0]=strtold(temp_token,NULL);
				while(temp_token!=NULL && i<dimension)
				{
					temp_token=strtok(NULL,",");
					if(temp_token!=NULL)	
					{
						newData[i]=strtold(temp_token,NULL);
						i++;
					}	
				}
			}
			else{///This is not working check why its not reading file in continuation
				//break;
				//printf("Sleeping\n");
				sleep(5);
				//printf("Checking file again\n");
				continue;
			}
		}	
			
			if(newpoints_arrived<nclusters && newCluster->centroidSet==0){
				
				if(rank==0)
				{	
					if(newpoints_arrived==0){
						int p=0;
						for(p=0;p<dimension;p++)
							centroidArray[p]=newData[p];
						newCluster->centroid=centroidArray;
						//printf("New cluster value set --only once for rank 0 allowed here :but rank %d\n",rank);
						newCluster->maxradius=0;
						My402ListAppend(&newCluster->datapoints,centroidArray);
						/*printf("Added first point to rank 0 list with following values\n");
						printf("centroid : %Lf\n",*newCluster->centroid);
						printf("maxRadius : %Lf\n",newCluster->maxradius);
						printf("List \n");
						My402ListElem *elem=My402ListFirst(&newCluster->datapoints);
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf\n",((long double*)(elem->obj))[l]);*/
						}
					else{
						//printf("sending second point\n");
						MPI_Send(&newData,dimension,MPI_LONG_DOUBLE,newpoints_arrived,10,MPI_COMM_WORLD);
						}

					centroidData *cd=(centroidData*)malloc(sizeof(centroidData));
					long double *c=(long double*)malloc(sizeof(long double)*dimension);
					int q=0;
					for(q=0;q<dimension;q++)
						c[q]=newData[q];
					cd->centroid=c;
					cd->rank=newpoints_arrived;
					My402ListAppend(&centroids_atroot,cd);
					newpoints_arrived++;
					/*if(newpoints_arrived==nclusters){
					printf("printing centroids\n");
					My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
					}
					}*/
					if(newpoints_arrived==nclusters)
					{
                        int j=0;
						My402ListElem *elem=NULL,*elem2;
						for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
						{
							for(elem2=My402ListNext(&centroids_atroot, elem);elem2!=NULL;elem2=My402ListNext(&centroids_atroot, elem2))
							{
								distance=0;
								for (j=0;j<dimension;j++)
								{
									distance+=(((long double*)(((centroidData*)elem->obj)->centroid))[j]-((long double*)(((centroidData*)elem2->obj)->centroid))[j])*(((long double*)(((centroidData*)elem->obj)->centroid))[j]-((long double*)(((centroidData*)elem2->obj)->centroid))[j]);	
								}
									distance=sqrt(distance);	
									if(global_radius==-1)
									{	global_radius=distance;
										printf("updating global radius first time to %Lf\n",global_radius);
									}
									else
									{
										if(global_radius>distance)
										{    global_radius=distance;	
											printf("updating global radius to %Lf\n",global_radius);
										}		
										else
											printf("not updating global radius as its value is %Lf\n",distance);														
									}
							}	
								
								
						}
					}
				}
				else
				{
					//printf("Waiting for recv=%d\n",rank);
					MPI_Recv(&newData,dimension,MPI_LONG_DOUBLE,0,10,MPI_COMM_WORLD,&status);
					int p=0;
					for(p=0;p<dimension;p++)
						centroidArray[p]=newData[p];
					newCluster->centroid=centroidArray;
					//printf("New cluster value set --only once for non rank 0 allowed here :but rank %d\n",rank);
					newCluster->maxradius=0;
					newCluster->centroidSet=1;
					My402ListAppend(&newCluster->datapoints,centroidArray);
					/*printf("Added first point to rank %d list with following values\n",rank);
					printf("centroid : %Lf\n",*newCluster->centroid);
					printf("maxRadius : %Lf\n",newCluster->maxradius);
					printf("List \n");
					My402ListElem *elem=My402ListFirst(&newCluster->datapoints);
					int l=0;
					for(l=0;l<dimension;l++)
						printf("value=%Lf\n",((long double*)(elem->obj))[l]);*/
				}
			//MPI_Bcast(&newpoints_arrived,1,MPI_INT,0,MPI_COMM_WORLD);
			//printf("new points=%d----rank=%d\n",newpoints_arrived,rank);
			}
			
			else{
				//printf("rank %d waiting before bcast\n",rank);
				MPI_Bcast(&global_radius,1,MPI_LONG_DOUBLE,0,MPI_COMM_WORLD);
				MPI_Bcast(&newData,dimension,MPI_LONG_DOUBLE,0,MPI_COMM_WORLD);
				//printf("received newdata at rank %d\n",rank);
				/*int m=0;
				for (m=0;m<dimension;m++){
					printf("newdata after centroid set at rank %d is %Lf\n",rank,newData[m]);
					printf("printing cluster centroid %Lf\n",newCluster->centroid[m]);
					}*/
				int j=0;
				long double sum=0;
				for (j=0;j<dimension;j++){
					sum+=(newData[j]-newCluster->centroid[j])*(newData[j]-newCluster->centroid[j]);		
				}	
				sum=sqrt(sum);
				//printf("SUM at rank %d =%Lf\n",rank,sum);
				in.radius_value=sum;
				in.proc_rank=rank;
				MPI_Allreduce(&in,&out,1,MPI_LONG_DOUBLE_INT,MPI_MINLOC,MPI_COMM_WORLD);
				if(out.proc_rank==rank){
					if(sum<=alpha*global_radius)
					{
						//printf("radius invariant maintained\n");
						if(newCluster->maxradius<sum)
							newCluster->maxradius=sum;
						long double *newMem=(long double*)malloc(sizeof(long double)*dimension);
						for(j=0;j<dimension;j++)
							newMem[j]=newData[j];
						My402ListAppend(&newCluster->datapoints,newMem);
					}
					else{
							//printf("radius invariant not maintained with sum=%Lf and global radius %Lf\n",sum, global_radius);
						onlinePhase=0;
						mergePhase=1;					
					}
					
					/*printf("lsit of all data points after adding new data point to cluster at rank %d is follows:\n",rank);
					My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf\n",((long double*)(elem->obj))[l]);
					}*/
				}
				MPI_Bcast(&onlinePhase,1,MPI_INT,out.proc_rank,MPI_COMM_WORLD);
				MPI_Bcast(&mergePhase,1,MPI_INT,out.proc_rank,MPI_COMM_WORLD);
				if(onlinePhase==0 && rank==0)
				{
					//printf("Created new cluster\n");
					extraCluster=(Cluster *)malloc(sizeof(Cluster));
    				extraCluster->number=nclusters;//as numbering starts from 0 
    				extraCluster->maxradius=0;
					extraCluster->centroidSet=1;
					int j=0;
					for(j=0;j<dimension;j++)
						extraClusterCentroid[j]=newData[j];
					extraCluster->centroid=extraClusterCentroid;
					centroidData *cdextra=(centroidData*)malloc(sizeof(centroidData));
					long double *cnew=(long double*)malloc(sizeof(long double)*dimension);
					int q=0;
					for(q=0;q<dimension;q++)
						cnew[q]=newData[q];
					cdextra->centroid=cnew;
					cdextra->rank=nclusters;
					My402ListAppend(&centroids_atroot,cdextra);
					/*printf("Created new cluster with centroid %Lf and %Lf\n", extraClusterCentroid[0],extraClusterCentroid[1]);
						printf("printing centroids\n");
					My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
					}*/
				}
		 	}	
		}
	while(mergePhase){
		printf("Starting merge phase by rank %d\n",rank);
		mergePhase=0;
		if(rank==0)
		{
			
		}
	}
	printf("Completing rank=%d\n",rank);
	
	MPI_Finalize();
return(0);
}
