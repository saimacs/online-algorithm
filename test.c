#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
int a;
int b;
}hello;

int main(int argc, char *argv[]){
	int rank,nprocs;
	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int data=10;
	int recvData;
	MPI_Status status;
	//MPI_File fh1;
	//MPI_File fh2;
	//MPI_File_open( MPI_COMM_WORLD, "testfileabcd1.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh1 );
	//MPI_File_open( MPI_COMM_WORLD, "testfileabcd2.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh2 );
	char * buf1=(char*)malloc(3*sizeof(char));
	char * buf2=(char*)malloc(5*sizeof(char));
	buf1="90";
	buf2="2120";
	int * read_buf1=(int*)malloc(sizeof(int));
	int * read_buf2=(int*)malloc(sizeof(int));
if(rank==0){
	int k=0;
	
	for(k=1;k<3;k++)
	{
		char namebuf[5]="test";
		char a[2];
		printf("File name is1\n");
		snprintf(a,2,"%d",k);
		printf("File name is2\n");
		strcat(namebuf,a);
		strcat(namebuf,".txt");
		printf("File name is %s\n",namebuf);
		FILE *fptry=fopen(namebuf,"w+");
		fclose(fptry);
	}
}
	if(rank==0)
	{
		char name[10]="test1.txt";
		MPI_File fh1;
		MPI_File_open( MPI_COMM_WORLD, name, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh1 );
		 int err= MPI_File_write( fh1, buf1, 2*sizeof(char), MPI_CHAR, &status );
		printf("By rank write done by 0 with err %d\n",err);
		MPI_File_close( &fh1 );
	}
	if(rank==1)
	{
		char name[10]="test2.txt";
		MPI_File fh2;
		MPI_File_open( MPI_COMM_WORLD, name, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh2 );
		int err= MPI_File_write( fh2, buf2, 4*sizeof(char), MPI_CHAR, &status );
		printf("By rank write done by 1 with err %d\n",err);
		MPI_File_close( &fh2 );
	}
	//MPI_File_close( &fh1 );
	//MPI_File_close( &fh2 );

//	MPI_File_open( MPI_COMM_WORLD, "testfileabcd2.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh2 );
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank==0)
	{
		MPI_File fh3;
		printf("1\n");
		MPI_File_open( MPI_COMM_WORLD, "test2.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh3 );
		printf("2\n");
		 MPI_File_read( fh3, read_buf2, 1, MPI_INT, &status );
		printf("By rank 0 %d\n",*read_buf2);
		MPI_File_close( &fh3 );
		
	}
//	MPI_File_close( &fh2 );
	//MPI_File_open( MPI_COMM_WORLD, "testfileabcd1.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh1 );
	//MPI_Barrier(MPI_COMM_WORLD);	
	if(rank==1)
	{
		MPI_File fh4;
		printf("3\n");
		MPI_File_open( MPI_COMM_WORLD, "test1.txt", MPI_MODE_RDWR, MPI_INFO_NULL, &fh4 );
		printf("4\n");
		 MPI_File_read( fh4, read_buf1, 1, MPI_INT, &status );
		printf("By rank 1 %d\n",*read_buf1);
		MPI_File_close( &fh4 );
	}
//		MPI_File_close( &fh1 );
	/*hello *testHello =(hello *)malloc(sizeof(hello));
	if(rank==0){
		int c=10;
		 testHello->a=c;
			testHello->b=90;
		//MPI_Send(&(testHello->a),sizeof(int),MPI_BYTE,1,10,MPI_COMM_WORLD);		
		MPI_Send(&testHello->b,sizeof(int),MPI_INT,1,20,MPI_COMM_WORLD);
	}	
	else{
		int d;
		//MPI_Recv(&d,sizeof(int),MPI_BYTE,0,10,MPI_COMM_WORLD,&status);
		MPI_Recv(&d,sizeof(int),MPI_INT,0,20,MPI_COMM_WORLD,&status);
		printf("d=%d\n",d);
		//printf("b=%d\n",(testHello->b));
	}*/
	MPI_Finalize();
}
