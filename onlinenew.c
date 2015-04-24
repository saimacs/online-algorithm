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
/*****PENDING:
CHECK ALL INVARIANTS ARE MAINTAINED...IF NOT INCORPORATE
ADD RANDOM VARIABLE IN FINDING GLOBAL RADIUS FOR THE FIRST TIME
SOLVE THE PROBLEM IF RANK 0 BREAKS, OTHER PROCESSES WAIT AT RECV OR BCAST WHEN ALL DATA POINTS IN FILE ARE OVER
CHECK IF SENDING OF CLUSTER DATA DURING MERGE CAN BE DONE USING ASYNCHRONOUS SENDS LIKE ISEND TO AVOID RACE CONDITIONS
add outer while loop over online and merge phases so that code continues with online phase again after merge
add conditions for all processors so that they exit from the outer while loop after all points are read.
check if any other variables being used in online phase and merge phase need to be reset to start the online phase again
is global radius between first k+1 points...then change it
//RESTART ONLINE PHASE AFTER MERGE PHASE
//FREE ANY OTHER DATA IF NOT REQUIRED while moving cluster data in merge phase
UPDATE MAX RADIUS OF EACH REMAINING CLUSTER AFTER MERGE OR DUING MERGE

*******/
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
int *mergeClusters;
int mergeReady;
}centroidData;

/*typedef struct merge_message{
char message;
int value;
}mergeMessage;*/

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
	My402List centroids_atroot,receiveList;
	memset(&centroids_atroot,0,sizeof(My402List));
	memset(&receiveList,0,sizeof(My402List));
	My402ListInit(&centroids_atroot);
	My402ListInit(&receiveList);
	FILE* fp=fopen("onlinedata.txt","r");
	long double centroidArray[dimension],extraClusterCentroid[dimension];
	int sendNewData[nclusters],clustersFull=0;//extraClusterAt=nclusters+1;//make changes to onlinephase with this array
	memset(sendNewData,0,sizeof(sendNewData));
	int MergeData[nclusters+1][nclusters+1];
	memset(MergeData,0,sizeof(MergeData));
	int moveExtraCluster=-1;
	long double newData[dimension];
	//int noMoreData=0;
if(rank==0){
	int k=0;

	for(k=0;k<nclusters+1;k++)
	{
		char namebuf[10]="test";
		char a[2];
		//printf("File name is1\n");
		snprintf(a,2,"%d",k);
		//printf("File name is2\n");
		strcat(namebuf,a);
		//printf("File name after a=%s is %s\n",a,namebuf);
		strcat(namebuf,".txt");
		//printf("File name is %s\n",namebuf);
		FILE *fptry=fopen(namebuf,"w+");
		fclose(fptry);
	}
	printf("leaving creation\n");
}
MPI_Barrier(MPI_COMM_WORLD);
	while(onlinePhase){
        printf("Starting online by rank %d\n",rank);
	    if(rank==0)
		{
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
//				noMoreData=1;
                break;
//                exit(0);
				//printf("Sleeping\n");
				//sleep(5);
				//printf("Checking file again\n");
				//continue;
			}
		}
//		MPI_Barrier(MPI_COMM_WORLD);
//        MPI_Bcast(&noMoreData,1,MPI_INT,0,MPI_COMM_WORLD);
//        if (noMoreData==1){
//        onlinePhase=0;
//        mergePhase=0;
//        break;
//        }

			//if(newpoints_arrived<nclusters && newCluster->centroidSet==0){
            if(rank==0 && clustersFull==0)
            {
                int k=0;
                for (k=0;k<=nclusters-1;k++)
                {
                    if(sendNewData[k]==0)
                    {
                        sendNewData[k]=1;
//                        printf("Sending to cluster %d\n",k);
                        break;
                    }
                    if(k==nclusters-1)
                    {
                        //printf("Clusters Full\n");

                        clustersFull=1;
                        //break;
                        if (global_radius==-1)//this condition is to compute global radius only for the first time....next iteration gets value form the current value
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
                                            //printf("updating global radius first time to %Lf\n",global_radius);
                                        }
                                        else
                                        {
                                            if(global_radius>distance)
                                            {    global_radius=distance;
                                                //printf("updating global radius to %Lf\n",global_radius);
                                            }
                                            else{
                                                //printf("not updating global radius as its value is %Lf\n",distance);
                                                }
                                        }
                                }
                            }
                        }
                        else{
                            global_radius*=beta;
                        }
                    }
                }

                //if (rank!=0)break;
                if(clustersFull==0)//WHAT IF CLUSTERSFULL==1
                {
                        if(k==0){
                            int p=0;
                            for(p=0;p<dimension;p++)
                                centroidArray[p]=newData[p];
                            newCluster->centroid=centroidArray;
                            //printf("New cluster value set --only once for rank 0 allowed here :but rank %d\n",rank);
                            newCluster->maxradius=0;
                            newCluster->centroidSet=1;
                            My402ListAppend(&newCluster->datapoints,centroidArray);
//                            printf("Added first point to rank 0 list with following values\n");
//                            printf("centroid : %Lf\n",*newCluster->centroid);
//                            printf("maxRadius : %Lf\n",newCluster->maxradius);
//                            printf("List \n");
//                            My402ListElem *elem=My402ListFirst(&newCluster->datapoints);
//                            int l=0;
//                            for(l=0;l<dimension;l++)
//                                printf("value=%Lf\n",((long double*)(elem->obj))[l]);
                            }
                        else{
//                            printf("sending point to %d\n",k);
                            MPI_Send(&newData,dimension,MPI_LONG_DOUBLE,k,10,MPI_COMM_WORLD);
                            }

                        centroidData *cd=(centroidData*)malloc(sizeof(centroidData));
                        long double *c=(long double*)malloc(sizeof(long double)*dimension);
                        int q=0;
                        for(q=0;q<dimension;q++)
                            c[q]=newData[q];
                        cd->centroid=c;
                        cd->rank=k;//WHAT WILL BE the rank and find all instances for newpoints arrived in the code
                        cd->mergeReady=0;
                        cd->mergeClusters =(int*)calloc(0,sizeof(int)*(nclusters+1));
                        //memset(&cd->mergeClusters,0,sizeof(cd->mergeClusters));
                        My402ListAppend(&centroids_atroot,cd);
                        //newpoints_arrived++;
//                        if(newpoints_arrived==nclusters){
//                        printf("printing centroids\n");
//                        My402ListElem *elem=NULL;
//                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//                            int l=0;
//                            for(l=0;l<dimension;l++)
//                                printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//                        }
//                        }
                    }
//                    else if(clustersFull==1){
//                        printf("printing centroids\n");
//                        My402ListElem *elem=NULL;
//                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//                            int l=0;
//                            for(l=0;l<dimension;l++)
//                                printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//                        }
//                    }
                }
               else if (rank!=0 && clustersFull==0)
                {
//                        printf("Waiting for recv=%d\n",rank);
                        MPI_Recv(&newData,dimension,MPI_LONG_DOUBLE,0,10,MPI_COMM_WORLD,&status);
                        int p=0;
                        for(p=0;p<dimension;p++)
                            centroidArray[p]=newData[p];
                        newCluster->centroid=centroidArray;
                        //printf("New cluster value set --only once for non rank 0 allowed here :but rank %d\n",rank);
                        newCluster->maxradius=0;
                        newCluster->centroidSet=1;
                        My402ListAppend(&newCluster->datapoints,centroidArray);
                        clustersFull=1;
                        continue;
//                        printf("Added first point to rank %d list with following values\n",rank);
//                        printf("centroid : %Lf\n",*newCluster->centroid);
//                        printf("maxRadius : %Lf\n",newCluster->maxradius);
//                        printf("List \n");
//                        My402ListElem *elem=My402ListFirst(&newCluster->datapoints);
//                        int l=0;
//                        for(l=0;l<dimension;l++)
//                            printf("value=%Lf\n",((long double*)(elem->obj))[l]);
                }
                //MPI_Bcast(&newpoints_arrived,1,MPI_INT,0,MPI_COMM_WORLD);
                //printf("new points=%d----rank=%d\n",newpoints_arrived,rank);

            if (clustersFull==1){
				//printf("rank %d waiting before bcast\n",rank);
				MPI_Bcast(&global_radius,1,MPI_LONG_DOUBLE,0,MPI_COMM_WORLD);
				MPI_Bcast(&newData,dimension,MPI_LONG_DOUBLE,0,MPI_COMM_WORLD);
//				printf("received newdata at rank %d\n",rank);
//				int m=0;
//				for (m=0;m<dimension;m++){
//					printf("newdata after centroid set at rank %d is %Lf\n",rank,newData[m]);
//					printf("printing cluster centroid %Lf\n",newCluster->centroid[m]);
//					}
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
                    //printf("Adding point to %d cluster in b phase\n",out.proc_rank);
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

//					printf("lsit of all data points after adding new data point to cluster at rank %d is follows:\n",rank);
//					My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(elem->obj))[l]);
//					}
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
					memset(&extraCluster->datapoints,0,sizeof(My402List));
                    My402ListInit(&extraCluster->datapoints);
					My402ListAppend(&extraCluster->datapoints,extraClusterCentroid);
					centroidData *cdextra=(centroidData*)malloc(sizeof(centroidData));
					long double *cnew=(long double*)malloc(sizeof(long double)*dimension);
					int q=0;
					for(q=0;q<dimension;q++)
						cnew[q]=newData[q];
					cdextra->centroid=cnew;
					cdextra->rank=nclusters;
					cdextra->mergeReady=0;
					cdextra->mergeClusters =(int*)calloc(0,sizeof(int)*(nclusters+1));
//					memset(&cdextra->mergeClusters,0,sizeof(cdextra->mergeClusters));
					My402ListAppend(&centroids_atroot,cdextra);
					printf("Created new cluster with centroid %Lf and %Lf\n", extraClusterCentroid[0],extraClusterCentroid[1]);
						printf("printing centroids\n");
					My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf and rank=%d\n",((long double*)(((centroidData*)elem->obj)->centroid))[l],((centroidData*)elem->obj)->rank);
					}
				}
		 	}
		}
		//printf("print by rank %d\n",rank);
		//MPI_Barrier(MPI_COMM_WORLD);

    MPI_File fh;
    char namebuf[10]="test";
    char a[2];
    char *buf=(char *)malloc(1024*sizeof(char));
    char *tempbuf=(char *)malloc(1024*sizeof(char));
    snprintf(a,2,"%d",rank);
    strcat(namebuf,a);
    strcat(namebuf,".txt");
    //printf("Before open namebuf=%s by rank %d\n",namebuf,rank);
    MPI_File_open( MPI_COMM_WORLD, namebuf, MPI_MODE_RDWR, MPI_INFO_NULL, &fh );
    //printf("After open\n");
    My402ListElem *elem=NULL;
    for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){

        int l=0;
        for(l=0;l<dimension;l++)
        {
            sprintf(tempbuf,"%Lf",((long double*)(elem->obj))[l]);
            if(l==0)
            {
                buf=tempbuf;
            }
            else
            {
                strcat(buf,tempbuf);
            }
            strcat(buf," ");
        }
        strcat(buf,"\n");
        //printf("Before write buf =%s\n",buf);
        int err= MPI_File_write(fh,buf,strlen(buf), MPI_CHAR, &status );
        //printf("After write\n");
        //free(buf);
        //free(tempbuf);
            //printf("value=%Lf\n",((long double*)(elem->obj))[l]);
    }

    //printf("By rank write done by 1 with err %d\n",err);
    MPI_File_close( &fh );
    if (rank==0)
    {
        MPI_File fh_extra;
        char namebufnew[10]="test";
        char a[2];
        char *bufnew=(char *)malloc(1024*sizeof(char));
        char *tempbufnew=(char *)malloc(1024*sizeof(char));
        snprintf(a,2,"%d",nclusters);
        strcat(namebufnew,a);
        strcat(namebufnew,".txt");
        printf("Before open namebufnew=%s by rank %d\n",namebufnew,rank);
        MPI_File_close( &fh_extra );
        printf("after test close\n");
    ????    MPI_File_open( MPI_COMM_WORLD, namebufnew, MPI_MODE_RDWR, MPI_INFO_NULL, &fh_extra );
        printf("After open\n");
        My402ListElem *elem=NULL;
        for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){

            int l=0;
            for(l=0;l<dimension;l++)
            {
                sprintf(tempbufnew,"%Lf",((long double*)(elem->obj))[l]);
                if(l==0)
                {
                    bufnew=tempbufnew;
                }
                else
                {
                    strcat(bufnew,tempbufnew);
                }
                strcat(bufnew," ");
            }
            strcat(bufnew,"\n");
            printf("Before write buf =%s\n",bufnew);
            int err= MPI_File_write(fh_extra,bufnew,strlen(bufnew), MPI_CHAR, &status );
            printf("After write\n");
            //free(buf);
            //free(tempbuf);
                //printf("value=%Lf\n",((long double*)(elem->obj))[l]);
        }

        printf("By rank 0 before close\n");
        MPI_File_close( &fh_extra );
        printf("By rank 0 after close\n");

    }

	while(mergePhase){

		printf("Starting merge phase by rank %d\n",rank);
		//global_radius*=beta;
		mergePhase=0;//REMOVE THIS
		if(rank==0)
		{
            int tieBreakArray[nclusters+1];
            memset(tieBreakArray,0,sizeof(tieBreakArray));
			int j=0;
			My402ListElem *elem=NULL,*elem2;
			printf("BETA DELTA=%Lf\n",beta*global_radius);

			for(elem=My402ListFirst(&centroids_atroot);elem!=NULL /*&& ((centroidData*)elem->obj)->mergeReady==0*/;elem=My402ListNext(&centroids_atroot, elem))
			{
                int recvCluster=((centroidData*)elem->obj)->rank;
//                if (MergeData[0][recvCluster]==20)
//                    continue;
				for(elem2=My402ListNext(&centroids_atroot, elem);elem2!=NULL /*&& ((centroidData*)elem2->obj)->mergeReady==0*/;elem2=My402ListNext(&centroids_atroot, elem2))
				{
                    int senderCluster=((centroidData*)elem2->obj)->rank;
//                    if (MergeData[0][senderCluster]==20)
//                        continue;
					distance=0;
					for (j=0;j<dimension;j++)
					{
						distance+=(((long double*)(((centroidData*)elem->obj)->centroid))[j]-((long double*)(((centroidData*)elem2->obj)->centroid))[j])*(((long double*)(((centroidData*)elem->obj)->centroid))[j]-((long double*)(((centroidData*)elem2->obj)->centroid))[j]);
					}
						distance=sqrt(distance);
						printf("Distance between %d and %d is %Lf\n",((centroidData*)elem->obj)->rank,((centroidData*)elem2->obj)->rank,distance);
						if(distance<=beta*global_radius)
						{
							((centroidData*)elem2->obj)->mergeReady=1;
							//printf("cluster %d is merge ready\n",((centroidData*)elem2->obj)->rank);
//							int rankStored=((centroidData*)elem2->obj)->rank;
//							((centroidData*)elem->obj)->mergeClusters[rankStored]=1;
//							printf("Merging cluster %d to %d\n",((centroidData*)elem2->obj)->rank, ((centroidData*)elem->obj)->rank);
//							int k=0;
//							for(k=0;k<nclusters+1;k++)
//								printf("merge cluster %d=%d\n",k,((centroidData*)elem->obj)->mergeClusters[k]);
//                            int senderCluster=((centroidData*)elem2->obj)->rank;
//                            int recvCluster=((centroidData*)elem->obj)->rank;
//                            MergeData[0][senderCluster]=20;//20 denotes this cluster is merged with some other cluster
//                            MergeData[1][senderCluster]=recvCluster;
//                            MergeData[0][recvCluster]=10;//10 denotes this cluster is master cluster to which some other clusters are merged
//                            MergeData[1][recvCluster]+=1;
                            if(tieBreakArray[senderCluster]!=1)
                            {
                                MergeData[recvCluster][senderCluster]=1;
                                tieBreakArray[senderCluster]=1;
                            }
						}
				}
			  }
                int h,f;
			  for(h=0;h<nclusters+1;h++)
			  {
                for(f=0;f<nclusters+1;f++)
                {
                    printf("%d ",MergeData[h][f]);
                }
                printf("\n");
			  }
			}

//            int s=0,t=0;
//            for (s=0;s<2;s++){
//                for (t=0;t<nclusters+1;t++){
//                    printf("%d ",MergeData[s][t]);
//                }
//                printf("\n");
//            }
       /* if(rank==0){
        MergeData[0][0]=10;
        MergeData[1][0]=50;
        MergeData[1][nclusters]=20;//REMOVE THIS AFTER TESTING
       MergeData[0][nclusters]=10;//REMOVE THIS AFTER TESTING
//       long double centroidArray1[dimension];
//       int p=0;
//       for(p=0;p<dimension;p++)
//                            centroidArray1[p]=100;
//                        //extraCluster->centroid=centroidArray1;
//                        //printf("New cluster value set --only once for non rank 0 allowed here :but rank %d\n",rank);
//                        extraCluster->maxradius=0;
//                        extraCluster->centroidSet=1;
//                        My402ListAppend(&extraCluster->datapoints,centroidArray1);
                        }*/
       MPI_Bcast(&MergeData,(2*(nclusters+1)),MPI_INT,0,MPI_COMM_WORLD);
//            if(rank==2){
//            int s=0,t=0;
//            for (s=0;s<2;s++){
//                for (t=0;t<nclusters+1;t++){
//                    printf("%d ",MergeData[s][t]);
//                }
//                printf("\n");
//            }}
   /* if(rank==2)//remove after testing
          {

                                  printf("Printing sending data of cluster\n");

		My402ListElem *elem=NULL;
					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
						int l=0;
						for(l=0;l<dimension;l++)
							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
					}
            MPI_Send(&newCluster->datapoints,sizeof(My402List),MPI_BYTE,0,0,MPI_COMM_WORLD);
          }*/

//    if(rank==0)
//    {
//        if(MergeData[0][0]==20)
//        {
//
//        }
//        if(MergeData[0][nclusters]==20)
//        {
//        }
//    }
//    else
//    {}
    }

       /***     if (rank==0){
                int totalRecv=0;
                My402ListElem *elem=NULL;
//                printf("max radius before begin %Lf\n",newCluster->maxradius);
//                printf("centroidset before begin %d\n",newCluster->centroidSet);
//                printf("Mergedata at rank 0 at 00= %d and at 01=%d\n",MergeData[0][0],MergeData[1][0]);
                if (MergeData[0][0]==20)
                {
                    if(MergeData[1][0]!=nclusters)
                    {
//                        printf("Sending to another node %d from rank 0 \n", MergeData[1][0]);
                        MPI_Send(&newCluster->datapoints,sizeof(My402List),MPI_BYTE,MergeData[1][0],MergeData[1][0],MPI_COMM_WORLD);
//                        printf("max radius before %Lf\n",newCluster->maxradius);
                        newCluster->maxradius=0;
//                        printf("max radius after %Lf\n",newCluster->maxradius);
//                        printf("centroidset before %d\n",newCluster->centroidSet);
                        newCluster->centroidSet=0;
//                        printf("centroidset after %d\n",newCluster->centroidSet);
//                        	printf("Printing data of cluster before \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}

                        My402ListUnlinkAll(&newCluster->datapoints);
//                        printf("Printing data of cluster after \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}




//                        printf("printing centroids before unlinking root\n");
//					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                        {
                            if(((centroidData *)elem->obj)->rank==0)
                                break;
                        }
                        My402ListUnlink(&centroids_atroot,elem);
//                        printf("printing centroids after unlinking root\n");
					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
//
//                        printf("Unlinked\n");

                    }
                    else if (MergeData[1][0]==nclusters)
                    {
//                        printf("Sending to extra cluster from rank 0\n");
                        My402ListElem *elem=NULL;
//                        printf("Printing data of extra cluster before \n");

                        //My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}

                        for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem))
                        {
                            My402ListAppend(&extraCluster->datapoints,elem->obj);
                        }

//                    printf("Printing data of extra cluster after \n");

                        //My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//                     printf("max radius before %Lf\n",newCluster->maxradius);
                        newCluster->maxradius=0;
//                        printf("max radius after %Lf\n",newCluster->maxradius);
//                        printf("centroidset before %d\n",newCluster->centroidSet);
                        newCluster->centroidSet=0;
//                        printf("centroidset after %d\n",newCluster->centroidSet);
//                                                	printf("Printing data of cluster before \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}

                        My402ListUnlinkAll(&newCluster->datapoints);
//                        printf("Printing data of cluster after \n");

		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//                        //My402ListElem *elem=NULL;
//                        printf("printing centroids before unlinking root\n");
//					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                        {
                            if(((centroidData *)elem->obj)->rank==0)
                                break;
                        }
                        My402ListUnlink(&centroids_atroot,elem);
//                                                printf("printing centroids after unlinking root\n");
//					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
//
//                        printf("Unlinked\n");
//                        printf("Printing data of extra cluster after after\n");
//
//                        //My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
                    }
                }
                if (MergeData[0][nclusters]==20)
                {
                    if(MergeData[1][nclusters]!=0)
                    {
                        My402ListElem *elem=NULL;
//                        printf("Sending to another node %d from rank %d \n", MergeData[1][0],nclusters);
                        MPI_Send(&extraCluster->datapoints,sizeof(My402List),MPI_BYTE,MergeData[1][nclusters],MergeData[1][nclusters],MPI_COMM_WORLD);
//                        printf("max radius before %Lf\n",extraCluster->maxradius);
                        extraCluster->maxradius=0;
//                        printf("max radius after %Lf\n",extraCluster->maxradius);
//                        printf("centroidset before %d\n",extraCluster->centroidSet);
                        extraCluster->centroidSet=0;
//                        printf("centroidset after %d\n",extraCluster->centroidSet);
//                        printf("Printing data of extra cluster before \n");
//                        for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}

                        My402ListUnlinkAll(&extraCluster->datapoints);
//                         printf("Printing data of extra cluster after \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}


//                        printf("printing centroids before unlinking ncluster\n");
					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                        {
                            if(((centroidData *)elem->obj)->rank==nclusters)
                                break;
                        }
                        My402ListUnlink(&centroids_atroot,elem);
                        //extraClusterMerged=1;

//                        printf("printing centroids after unlinking ncluster\n");
//					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//                        }
                    }
                    else if (MergeData[1][nclusters]==0)
                    {
                        My402ListElem *elem=NULL;
//                        printf("Printing data of root cluster before \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
                        for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem))
                        {
                            My402ListAppend(&newCluster->datapoints,elem->obj);
                        }

//                        printf("Printing data of root cluster after \n");
//
//		//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//					printf("max radius before %Lf\n",extraCluster->maxradius);
                        extraCluster->maxradius=0;
//                        printf("max radius after %Lf\n",extraCluster->maxradius);
//                        printf("centroidset before %d\n",extraCluster->centroidSet);
                        extraCluster->centroidSet=0;
//                        printf("centroidset before %d\n",extraCluster->centroidSet);
//                        printf("Printing data of cluster before \n");
//                        for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}

                        My402ListUnlinkAll(&extraCluster->datapoints);
//                        printf("Printing data of cluster after \n");
////                        for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
////						int l=0;
////						for(l=0;l<dimension;l++)
////							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
////					}
//                        //My402ListElem *elem=NULL;
//                        printf("printing centroids before unlinking root\n");
					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                        {
                            if(((centroidData *)elem->obj)->rank==nclusters)
                                break;
                        }
                        My402ListUnlink(&centroids_atroot,elem);
//                        printf("printing centroids after unlinking root\n");
					//My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}

//                        printf("Unlinked\n");
                    }
                }
                 if (MergeData[0][0]==10)
                {
                    totalRecv+=MergeData[1][0];
                }
                if (MergeData[0][nclusters]==10)
                {
                    totalRecv+=MergeData[1][nclusters];
                }
//                printf("TOTAL RECV %d\n",totalRecv);
****************************************TESTED TILL HERE >>>ALL ABOVE CODE*******************TEST BELOW CODE***********
******How to send and recv linked lists change the MPI sends above which are sending lists if send and recv of linked lists are modified******
//               while(totalRecv>0){
//                    printf("Waiting for recv at rank %d\n",rank);
//                    int number_amount;
//                    MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
//                    MPI_Get_count(&status, MPI_BYTE, &number_amount);
//                    printf("Number amount =%d\n",number_amount);
//                    long double *lista=(long double*)malloc(sizeof(long double)*number_amount);
//                    MPI_Recv(&lista,sizeof(My402List)*10,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
//                    printf("Printing data of recv list\n");
//                    My402ListElem *elem1=NULL;
//                    printf("Printing data of recv list1 \n");
//                        for(elem1=My402ListFirst(&lista);elem1!=NULL;elem1=My402ListNext(&lista, elem1)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem1->obj))[l],rank);}
//                    if(status.MPI_TAG==0)
//                    {
//                        printf("Tag 0 appending to main cluster at root\n");
//                        printf("Printing data of cluster before \n");
//
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//
//                        for(elem=My402ListFirst(&receiveList);elem!=NULL;elem=My402ListNext(&receiveList, elem))
//                        {
//                            My402ListAppend(&newCluster->datapoints,elem->obj);
//                        }
//                        My402ListUnlinkAll(&receiveList);
//
//                        printf("Printing data of cluster after \n");
//
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//
//                    }
//                    else if(status.MPI_TAG==nclusters)
//                    {
//                        printf("Tag ncluster appending to extra cluster at root\n");
//                        My402ListElem *elem=NULL;
//                        printf("Printing data of cluster before \n");
//
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//                        for(elem=My402ListFirst(&receiveList);elem!=NULL;elem=My402ListNext(&receiveList, elem))
//                        {
//                            My402ListAppend(&extraCluster->datapoints,elem);
//                        }
//                        My402ListUnlinkAll(&receiveList);
//                        printf("Printing data of cluster after \n");
//
//					for(elem=My402ListFirst(&extraCluster->datapoints);elem!=NULL;elem=My402ListNext(&extraCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//                        }
//                    }
                    totalRecv--;
                    printf("TOTAL RECV dec %d\n",totalRecv);
                }***/



        /*****    else{
                if(MergeData[0][rank]==10)
                {
                    int totalRecv=MergeData[1][rank];
                    while(totalRecv>0){
                        MPI_Recv(&receiveList,sizeof(My402List),MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
                        My402ListElem *elem=NULL;
                        for(elem=My402ListFirst(&receiveList);elem!=NULL;elem=My402ListNext(&receiveList, elem))
                        {
                            My402ListAppend(&newCluster->datapoints,elem);
                        }
                        My402ListUnlinkAll(&receiveList);
                        totalRecv--;
                    }

                }
                else if (MergeData[0][rank]==20)
                {
                        int destination=MergeData[1][rank];
                        if(destination==nclusters)
                            destination=0;
                        MPI_Send(&newCluster->datapoints,sizeof(My402List),MPI_BYTE,destination,MergeData[1][rank],MPI_COMM_WORLD);
                        newCluster->maxradius=0;
                        newCluster->centroidSet=0;
                        My402ListUnlinkAll(&newCluster->datapoints);
                        My402ListElem *elem=NULL;
                        for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                        {
                            if(elem->rank==rank)
                                break;
                        }
                        My402ListUnlink(&centroids_atroot,elem);
                }
            }

            if (rank==0)
            {
                int j=0,lessClusters=0;
                for (j=0;j<nclusters;j++)
                {
                    if(MergeData[0][j]==20)
                    {
                        sendNewData[j]=0;
                        lessClusters=1;
                    }
                }
                if(lessClusters==1)
                {
                    clustersFull=0;
                    mergePhase=0;
                    onlinePhase=1;
                }
                else{
                    mergePhase=1;
                    onlinePhase=0;
                }
                if(MergeData[0][nclusters]!=20)
                {
                    for(j=0;j<nclusters;j++)
                    {
                        if (sendNewData[j]==0)
                        {
                            moveExtraCluster=j;
                            break;
                        }

                    }
                }
             }
            MPI_Bcast(&moveExtraCluster,1,MPI_INT,0,MPI_COMM_WORLD);
            if(rank==0){
                if(moveExtraCluster!=-1)
                {
                    MPI_Send(&extraCluster->centroid,sizeof(My402List),MPI_BYTE,moveExtraCluster,0,MPI_COMM_WORLD);
                    My402ListElem *elem=NULL;
                    for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
                    {
                        if(elem->rank==nclusters)
                            break;
                    }
                    My402ListUnlink(&centroids_atroot,elem);
                    free(extraCluster->centroid);
                    free(extraCluster);
                }
                moveExtraCluster=-1;
            }
            else{
                if (moveExtraCluster!=-1 && moveExtraCluster==rank){
                        MPI_Recv(&newData,dimension,MPI_LONG_DOUBLE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

                        newCluster->maxradius=0;
                        newCluster->centroidSet=1;
                        newCluster->number=rank;
                        long double *c=(long double*)malloc(sizeof(long double)*dimension);
                        int p=0;
                        for(p=0;p<dimension;p++)
                            c[p]=newData[p];
                        newCluster->centroid=c;
                        centroidData *cd=(centroidData*)malloc(sizeof(centroidData));
                        cd->centroid=c;
                        cd->rank=rank;
                        cd->mergeReady=0;
                        cd->mergeClusters =(int*)calloc(0,sizeof(int)*(nclusters+1));
                        //memset(&cd->mergeClusters,0,sizeof(cd->mergeClusters));
                        My402ListAppend(&centroids_atroot,cd);
                }
            }
            MPI_Bcast(&moveExtraCluster,1,MPI_INT,0,MPI_COMM_WORLD);
            MPI_Bcast(&onlinePhase,1,MPI_INT,0,MPI_COMM_WORLD);
            MPI_Bcast(&mergePhase,1,MPI_INT,0,MPI_COMM_WORLD);*/
   // }

			/*for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem))
			{
					int p=0,counter, M[2];
					if(((centroidData*)elem->obj)->mergeReady!=1)
					{
						for (p=0;p<nclusters+1;p++)
						{
							if(((centroidData*)elem->obj)->mergeClusters[p]==1)
							{
								counter++;
								//mergeMessage *m=(mergeMessage*)malloc(sizeof(mergeMessage));
								M[0]=1;//1 meaning Send type
								M[1]=((centroidData*)elem->obj)->rank;
								MPI_Send(&M,2,MPI_INT,p,10,MPI_COMM_WORLD);

							}
						}
					M[0]=2;//2 means accpet type
					M[1]=counter;
					MPI_Send(&M,2,MPI_INT,((centroidData*)elem->obj)->rank,10,MPI_COMM_WORLD);
					}

					//else if(//WHAT if rank==0)
			}

		}
		else{//if rank !=0
			MPI_Recv(&MessageNonZero, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,MPI_COMM_WORLD, &status)
		}
			MPI_Barrier(MPI_COMM_WORLD);*/

//	printf("Printing data of all clusters by rank %d\n",rank);
//
//		My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&newCluster->datapoints);elem!=NULL;elem=My402ListNext(&newCluster->datapoints, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf on rank %d\n",((long double*)(elem->obj))[l],rank);
//					}
//    if(rank==0){
//        printf("Data on new cluster at rank %d\n",rank);
//        		//My402ListElem *elem=NULL;
//					printf("Created new cluster with centroid %Lf and %Lf\n", extraCluster->centroid[0],extraCluster->centroid[1]);
//					printf("printing centroids\n");
//					My402ListElem *elem=NULL;
//					for(elem=My402ListFirst(&centroids_atroot);elem!=NULL;elem=My402ListNext(&centroids_atroot, elem)){
//						int l=0;
//						for(l=0;l<dimension;l++)
//							printf("value=%Lf\n",((long double*)(((centroidData*)elem->obj)->centroid))[l]);
//					}
//    }
	printf("Completing rank=%d\n",rank);
	MPI_Finalize();
return(0);
}
