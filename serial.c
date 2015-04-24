#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "my402list.h"
#include "cs402.h"
#include <math.h>
#include <unistd.h>

typedef struct cluster_struct{
long double *centroid;
long double maxradius;
int number;
int centroidSet;//CHekc centoir nulll
My402List datapoints;
}Cluster;

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
	int i=0;
	Cluster *clusterArray;
	clusterArray=(Cluster*)malloc((nclusters+1)*sizeof(Cluster));
	for (i=0;i<nclusters+1;i++)
    {

		clusterArray[i].number=i;
		memset(&clusterArray[i].datapoints,0,sizeof(My402List));
		My402ListInit(&clusterArray[i].datapoints);
		clusterArray[i].centroid=(long double*)calloc(sizeof(long double)*dimension,sizeof(long double));
		clusterArray[i].maxradius=0;
		clusterArray[i].centroidSet=0;
		//printf("Created cluster %d\n",clusterArray[i].number);
	}
    int onlinePhase=1;
    int mergePhase=0;
	long double global_radius=-1, distance=0;
	int clustersFull=0;
	int noMoreData=0;
	FILE* fp=fopen("onlinedata.txt","r");
while(1){
	while(onlinePhase){
		long double newData[dimension];
		char *temp_token,buf[1000];
		//printf("...checking\n");
		if(fgets(buf,1000,fp)!=NULL)
		{
			//printf("BUF=%s\n",buf);
			int i=1;
			temp_token=strtok(buf,",");
			newData[0]=strtold(temp_token,NULL);
			while(temp_token!=NULL && i<dimension)
			{
				temp_token=strtok(NULL,",");
				if(temp_token!=NULL)
				{
					newData[i]=strtold(temp_token,NULL);
					//printf("%Lf\n",newData[i]);
					i++;
				}
			}
		}
		else{///This is not working check why its not reading file in continuation
				noMoreData=1;
				onlinePhase=0;
				mergePhase=0;
                break;//SOLVE THE PROBLEM IF RANK 0 BREAKS, OTHER PROCESSES WAIT AT RECV OR BCAST
//                exit(0);
				//printf("Sleeping\n");
				//sleep(5);
				//printf("Checking file again\n");
				//continue;
			}

            if (clustersFull==1)
            {
                 int m=0,j=0,minCluster=-1;
                 long double minDist=-1;
                            //My402ListElem *elem=NULL,*elem2;
                for(m=0;m<nclusters;m++)
                {
                    distance=0;
                    for (j=0;j<dimension;j++)
                    {
                        distance+=(((clusterArray[m].centroid))[j]-newData[j])*(((clusterArray[m].centroid))[j]-newData[j]);
                    }
                        distance=sqrt(distance);
                       //printf("dist between cluster %d and new point is %Lf\n",m,distance);
                        if(minDist==-1)
                        {	minDist=distance;
                            minCluster=m;
//                                            printf("updating global radius first time to %Lf\n",global_radius);
                        }
                        else if (minDist>distance)
                        {
                                minDist=distance;
                                minCluster=m;
//                                                printf("updating global radius to %Lf\n",global_radius);
                        }
                }
                long double *d=(long double*)malloc(sizeof(long double)*dimension);
                int f=0;
                for(f=0;f<dimension;f++)
                    d[f]=newData[f];
                if(minDist<=alpha*global_radius)
                {
                    My402ListAppend(&clusterArray[minCluster].datapoints,d);
                    if(minDist>clusterArray[minCluster].maxradius)
                        clusterArray[minCluster].maxradius=minDist;
                    //printf("Addin new point to cluster %d\n",minCluster);
                }
                else{
                    clusterArray[nclusters].centroid=d;
                    clusterArray[nclusters].centroidSet=1;
                    clusterArray[nclusters].maxradius=0;
                    My402ListAppend(&clusterArray[nclusters].datapoints,d);
                    //printf("Created new cluster GA=%Lf and dist=%Lf\n",global_radius*alpha,minDist);
                    onlinePhase=0;
                    mergePhase=1;
                }


            }
			//if(newpoints_arrived<nclusters && newCluster->centroidSet==0){
            if(clustersFull==0)
            {
                int k=0;
                for (k=0;k<nclusters;k++)//CHANGE EXTRA CLUSTER POSITION AFTER MERGE PHASE
                {
                    if(clusterArray[k].centroidSet==0)
                    {
						long double *c=(long double*)malloc(sizeof(long double)*dimension);
                        int q=0;
                        for(q=0;q<dimension;q++)
                            c[q]=newData[q];
						clusterArray[k].centroid=c;
						clusterArray[k].centroidSet=1;
                        clusterArray[k].maxradius=0;
                        My402ListAppend(&clusterArray[k].datapoints,c);
//                        printf("clusters filled %d\n",k);
                        break;
                    }
                }
//                printf("k value %d\n",k);
                if(k==nclusters-1)
                {
//                        printf("Clusters Full----------\n");

                        clustersFull=1;
                        //break;
                        if (global_radius==-1)//this condition is to compute global radius only for the first time....next iteration gets value form the current value
                        {
                            int m=0,n=0,j=0;
                            //My402ListElem *elem=NULL,*elem2;
                            for(m=0;m<nclusters+1;m++)
                            {
                                for(n=m+1;n<nclusters+1;n++)
                                {
                                    distance=0;
                                    for (j=0;j<dimension;j++)
                                    {
                                        distance+=(((long double*)(clusterArray[m].centroid))[j]-((long double*)(clusterArray[n].centroid))[j])*(((long double*)(clusterArray[m].centroid))[j]-((long double*)(clusterArray[n].centroid))[j]);
                                    }
                                        distance=sqrt(distance);
//                                        printf("global radius between cluster %d and %d is %Lf\n",m,n,distance);
                                        if(global_radius==-1)
                                        {	global_radius=distance;
//                                            printf("updating global radius first time to %Lf\n",global_radius);
                                        }
                                        else if (global_radius>distance)
                                        {
                                                global_radius=distance;
//                                                printf("updating global radius to %Lf\n",global_radius);
                                        }
                                }
                            }
                            double r=(fmod(rand(),(1+1-(1/M_E))))+(1/M_E);
                            global_radius*=r;
                        }
//                        else{
//                            global_radius*=beta;//NOT TESTED THIS
//                            printf("Multiplying global radius with beta\n");
//                        }
                    }
            }
   }
	while(mergePhase){//RESTART ONLINE PHASE AFTER MERGE PHASE

		int clustersFree=0;
		int firstFreeCluster=-1;
		global_radius=beta*global_radius;
			printf("BETA DELTA=%Lf\n",global_radius);

			int m=0,n=0,j=0,d=0;
			long double maxradiusDistance=0;

            for(m=0;m<nclusters+1 && clusterArray[m].centroidSet!=0;m++)
            {
                for(n=m+1;n<nclusters+1 && clusterArray[n].centroidSet!=0;n++)
                {
                    distance=0;
                    for (j=0;j<dimension;j++)
                    {
                        distance+=(((long double*)(clusterArray[m].centroid))[j]-((long double*)(clusterArray[n].centroid))[j])*(((long double*)(clusterArray[m].centroid))[j]-((long double*)(clusterArray[n].centroid))[j]);
                    }
                    distance=sqrt(distance);
                    if(distance<=global_radius)
                    {   printf("Distance between clusters is %Lf and bound if %Lf\n",distance,global_radius);
                        printf("Moving cluster %d to %d\n",n,m);
                        My402ListElem *elem;
                        for(elem=My402ListFirst(&clusterArray[n].datapoints);elem!=NULL;elem=My402ListNext(&clusterArray[n].datapoints, elem))
                        {
                            long double *e=(long double*)malloc(sizeof(long double)*dimension);
                            int r=0;
                            for(r=0;r<dimension;r++)
                                e[r]=((long double *)(elem->obj))[r];
                            maxradiusDistance=0;
                            for (d=0;d<dimension;d++)
                            {
                                  maxradiusDistance+=(((long double*)(clusterArray[m].centroid))[d]-e[d])*(((long double*)(clusterArray[m].centroid))[d]-((long double*)(elem->obj))[d]);
                            }
                            maxradiusDistance=sqrt(maxradiusDistance);
                            printf("Current max radius is %Lf and new maxradius is %Lf\n",clusterArray[m].maxradius,maxradiusDistance);
                            if (maxradiusDistance>clusterArray[m].maxradius)
                            {
                                clusterArray[m].maxradius=maxradiusDistance;
                                printf("Updating max radius of cluster merging points %d\n",m);
                            }
                            My402ListAppend(&clusterArray[m].datapoints,e);

                        }
                        My402ListUnlinkAll(&clusterArray[n].datapoints);
                        clusterArray[n].maxradius=0;
                        clusterArray[n].centroidSet=0;
                        free(clusterArray[n].centroid);
                        clustersFree++;
                        if (firstFreeCluster==-1)
                            firstFreeCluster=n;
                    }

                }
            }

            if (clusterArray[nclusters].centroidSet!=0 && firstFreeCluster!=-1)
            {
               long double *c=(long double*)malloc(sizeof(long double)*dimension);
               int q=0;
               for(q=0;q<dimension;q++)
                    c[q]=clusterArray[nclusters].centroid[q];
               clusterArray[firstFreeCluster].centroid=c;
               clusterArray[firstFreeCluster].centroidSet=1;
               clusterArray[firstFreeCluster].maxradius=0;
               My402ListAppend(&clusterArray[firstFreeCluster].datapoints,c);
               clusterArray[nclusters].maxradius=0;
               clusterArray[nclusters].centroidSet=0;
               free(clusterArray[nclusters].centroid);
               My402ListUnlinkAll(&clusterArray[nclusters].datapoints);
               clustersFree--;
            }
            else if (clusterArray[nclusters].centroidSet==0)
                clustersFree--;
            if (clusterArray[nclusters].centroidSet==0)
            {
                onlinePhase=1;
                mergePhase=0;
                if (clustersFree>0)
                clustersFull=0;
            }


        }
        if(noMoreData==1)
            break;
    }
printf("Printing data of all clusters\n");
for (i=0;i<nclusters+1;i++){
        printf("Cluster %d data\n",i);
        printf("centroidSet: %d\n",clusterArray[i].centroidSet);
        printf("centroid:\n");
        int l=0;
        for(l=0;l<dimension;l++)
			printf("value=%Lf\n",((long double*)(clusterArray[i].centroid))[l]);
        printf("Data points:\n");
        My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&clusterArray[i].datapoints);elem!=NULL;elem=My402ListNext(&clusterArray[i].datapoints, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf\n",((long double*)(elem->obj))[l]);
					}
            }
return(0);
}

