#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mpi.h> 
#include <math.h>
#include <string.h>
#include <assert.h>
/*Tags*/
#define SERVER 0 
#define START_LEADER_ELECTION 1
#define CONNECT 2 
#define PRINT 3
#define ORDER 4 
#define SUPPLY 5
#define EXTERNAL_SUPPLY 6
#define REPORT 7
#define ACK 8
#define LEADER_ELECTION_DONE 9
#define CLIENT 10
#define CHILD 11
#define TERMINATE 13
#define PROBE 14
#define REPLY 15
#define SUPPLY_REQUEST 16

void array_copy(int array1[5],int array2[5]){
	for(int i=0;i<5;i++)
		array1[i]=array2[i];
}
int main(int argc, char *argv[]){
	int zero_array[5];
	zero_array[0]=0;
	zero_array[1]=0;
	zero_array[2]=0;
	zero_array[3]=0;
	zero_array[4]=0;
	int rank, world_size, i;
	/** MPI Initialisation **/
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Status status;
	
	MPI_Datatype MESSAGE_ARRAY;
	MPI_Type_contiguous(5, MPI_INT, &MESSAGE_ARRAY);
	MPI_Type_commit(&MESSAGE_ARRAY);
	
	if (argc <= 2) {
		fprintf(stderr, "Insufficient number of arguments given,exiting\n");
		exit(-1);
	}
	int NUM_SERVERS = atoi(argv[2]);
	char* file_name = argv[1];
	int  row[5];
	int  message_array[300][5];
	int leader;
	
	if(rank == 0){
		// Coordinator
		//printf("[rank: %d] Coordinator started\n", rank);
		int id[NUM_SERVERS];//rank twn servers
		/*Test file Reading*/
		/*--------------------------------*/
		FILE * fp;
		char * line = NULL;
		size_t len = 0;
		fp = fopen(file_name, "r");
		if (fp == NULL) {
			printf("Cant open test file\nExiting\n");
			exit(-1);
		}
		int index=0;
		while (getline(&line, &len, fp)!= -1) {
					row[0]=0;//clean up row array
					row[1]=0;
					row[2]=0;
					row[3]=0;
					row[4]=0;	
			char *p = strtok(line, "\n");
			p = strtok(p," ");

			int flag=1;
			if (!strcmp(p,"SERVER")) row[0]=0;
			else if (!strcmp(p,"CONNECT")) row[0]=2;
			else if (!strcmp(p,"ORDER")) row[0]=4;
			else if (!strcmp(p,"SUPPLY")) row[0]=5;
			else if (!strcmp(p,"EXTERNAL_SUPPLY")) row[0]=6;
			//else if (!strcmp(p,"REPORT")) {row[0]=7;flag=0;}
			else if (p[0]=='R') {row[0]=7;flag=0;}
			else if (p[0]=='S') {row[0]=1; flag=0;}
			//else if (!strcmp(p,"START_LEADER_ELECTION")) {row[0]=1; flag=0;}
			//else if (!strcmp(p,"PRINT")) {row[0]=3;flag=0;}
			else if (p[0]=='P') {row[0]=3;flag=0;}
			else {
				printf("Unrecognizable message type:%s\nExiting\n",p);
				exit(-1);
			}
			int index2=1;

			while(p != NULL && flag) {
				p = strtok(NULL, " ");
				//printf("index2: %d %s\n",index2, p);
				if (p!=NULL)
					row[index2++]=atoi(p);
			}

			array_copy(message_array[index],row);	
			index++;		
		}
		printf("Test file read successfully.\n");
		fclose(fp);
		/*
		for(int i=0;i<index;i++){
			printf("%d %d %d %d %d\n",message_array[i][0],message_array[i][1],message_array[i][2],message_array[i][3],message_array[i][4]);
		}
		/*--------------------------------*/
		array_copy(row,zero_array);
		int active_events=0;
		/*Ftiaxnei tus servers*/
		for(int i = 0; i< NUM_SERVERS; i++){
			array_copy(row,message_array[i]);
			MPI_Send(&row, 1, MESSAGE_ARRAY, i+1, SERVER, MPI_COMM_WORLD);
			active_events++;
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			active_events--;
			assert(status.MPI_TAG==ACK);
			assert(status.MPI_SOURCE==i+1);
		}

		/*Ftiaxnei ton server leader*/	
		for(int i=0;i<NUM_SERVERS;i++){
			active_events++;
			array_copy(row,message_array[i]);
			MPI_Send(&row, 1, MESSAGE_ARRAY, i+1, START_LEADER_ELECTION, MPI_COMM_WORLD);
		}
		while(active_events>0){		
		MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    	active_events--;}
		assert(status.MPI_TAG==LEADER_ELECTION_DONE);
		leader=row[3];

		for(int i=0;i<NUM_SERVERS;i++){
			MPI_Send(&row, 1, MESSAGE_ARRAY, i+1, LEADER_ELECTION_DONE, MPI_COMM_WORLD);
			active_events++;
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			active_events--;
			assert(status.MPI_TAG==ACK);
			assert(status.MPI_SOURCE==i+1);
		}
		/*stelnei to CLIENT se olous tous clients*/

		for(int i=NUM_SERVERS+1;i<world_size;i++){//i<=world_size
			MPI_Send(&row, 1, MESSAGE_ARRAY, i, CLIENT, MPI_COMM_WORLD);
			active_events++;
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			active_events--;
			assert(status.MPI_TAG == ACK);
			assert(status.MPI_SOURCE==i);
		}

		/*stelnei to CONNECT se olous tous clients*/
		for(int i=NUM_SERVERS+1;i<world_size;i++){
			array_copy(row,message_array[i]);
			MPI_Send(&row, 1, MESSAGE_ARRAY, message_array[i][1], CONNECT, MPI_COMM_WORLD);
			active_events++;
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			active_events--;
			assert(status.MPI_TAG == ACK);
			assert(status.MPI_SOURCE==message_array[i][1]);
		}
		int current_index=world_size;//prohgeitai to leader election 
		while (current_index<index){
			//printf("%d %d %d %d %d\n",message_array[current_index][0],message_array[current_index][1],message_array[current_index][2],message_array[current_index][3],message_array[current_index][4]);
			array_copy(row,zero_array);
			if(message_array[current_index][0]==ORDER){
				array_copy(row,message_array[current_index]);
				MPI_Send(&row, 1, MESSAGE_ARRAY, message_array[current_index][1], ORDER, MPI_COMM_WORLD);
				active_events++;
				MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				active_events--;
				assert(status.MPI_TAG == ACK);
				assert(status.MPI_SOURCE==message_array[current_index][1]);
				//printf("<---------------------------------------------->\n");
			}else if(message_array[current_index][0]==SUPPLY){	
				array_copy(row,message_array[current_index]);		
				MPI_Send(&row, 1, MESSAGE_ARRAY, message_array[current_index][1], SUPPLY, MPI_COMM_WORLD);
				active_events++;
				MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				active_events--;
				assert(status.MPI_TAG == ACK);
				assert(status.MPI_SOURCE==message_array[current_index][1]);
				//printf("<---------------------------------------------->\n");
			}else if(message_array[current_index][0]==EXTERNAL_SUPPLY){
				if (active_events==0){
					array_copy(row,message_array[current_index]);
					MPI_Send(&row, 1, MESSAGE_ARRAY, leader, EXTERNAL_SUPPLY, MPI_COMM_WORLD);
					active_events++;
					MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					active_events--;
					assert(status.MPI_TAG == ACK);
					assert(status.MPI_SOURCE==leader);
					//printf("<---------------------------------------------->\n");
				}else
					continue;
			}else if(message_array[current_index][0]==PRINT){
				if (active_events==0){
					array_copy(row,message_array[current_index]);
					MPI_Send(&row, 1, MESSAGE_ARRAY, leader, PRINT, MPI_COMM_WORLD);
					active_events++;
					MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					active_events--;
					assert(status.MPI_TAG == ACK);
					assert(status.MPI_SOURCE==leader);	
					//printf("<---------------------------------------------->\n");
				}else 
					continue;	
			}else if(message_array[current_index][0]==REPORT){
				array_copy(row,message_array[current_index]);
				MPI_Send(&row, 1, MESSAGE_ARRAY, leader, REPORT, MPI_COMM_WORLD);
				active_events++;
				MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				active_events--;
				assert(status.MPI_TAG == ACK);
				assert(status.MPI_SOURCE==leader);
				//printf("<---------------------------------------------->\n");
			}
			sleep(0.5);
			sleep(0.5);
			current_index++;
		}
		while(active_events>0){;}// perimene mexri an teleiwsoun oloi

		//stelnei se olous TERMINATE
		for(int i=1;i<world_size;i++){
			MPI_Send(&row, 1, MESSAGE_ARRAY, i, TERMINATE, MPI_COMM_WORLD);	
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);		
			assert(status.MPI_TAG == ACK);
			assert(status.MPI_SOURCE==i);		
		}
		
		printf("Exiting succesfully.\n");
		sleep(1);
		MPI_Abort(MPI_COMM_WORLD,0);
		

	}else if (rank <=NUM_SERVERS){
		// Servers
		//printf("[rank: %d] Server started\n", rank);
		int stock=300;
		int right_server_rank=-1;
		int left_server_rank=-1;
		int * children=malloc(sizeof(int));
		int children_count=0;
		int tree_received_stock=0;
		int has_started_leader_election=0;
		int already_received_reply_from_right=0;
		int already_received_reply_from_left=0;

		while(1){
			array_copy(row,zero_array);
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE,MPI_ANY_TAG , MPI_COMM_WORLD, &status);
			if(status.MPI_TAG==SERVER){
				assert(rank==row[1]);
				assert(status.MPI_SOURCE==0);
				left_server_rank=row[2];
				right_server_rank=row[3];
				MPI_Send(&row, 1, MESSAGE_ARRAY, 0, ACK, MPI_COMM_WORLD);
			}else if(status.MPI_TAG==CHILD){
				children[children_count++]=status.MPI_SOURCE;
				children=realloc(children,sizeof(int)*children_count+1);	
				MPI_Send(&row, 1, MESSAGE_ARRAY, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);		
			}else if(status.MPI_TAG==ORDER){
				if(stock-row[2]<0) {
					row[2]=stock;
					stock=0;
				}else					
					stock-=row[2];

				MPI_Send(&row, 1, MESSAGE_ARRAY, row[1], ACK, MPI_COMM_WORLD);	
			}else if(status.MPI_TAG==SUPPLY){
				if(row[1]==rank){//an einai o prwtos
					if(status.MPI_SOURCE==right_server_rank){
						stock+=(row[3]-row[2]);
						printf("SERVER <%d> RECEIVED <%d>\n",row[1],row[3]-row[2]);
						
						MPI_Send(&row, 1, MESSAGE_ARRAY, 0,ACK, MPI_COMM_WORLD);
					}else{
						row[3]=row[2];
						MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, SUPPLY, MPI_COMM_WORLD);					
						
					}
				}else {		
					if(stock>row[2]+150){
						stock=stock-row[2];
						row[2]=0;
					}else if(stock>150&&stock<row[2]+150){
						row[2]-=stock-150;
						stock=150;
					}
					
				
					MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, SUPPLY, MPI_COMM_WORLD);
				}

			}else if(status.MPI_TAG==EXTERNAL_SUPPLY){
				if (leader==rank&&status.MPI_SOURCE==right_server_rank){
					stock+=row[1];
					printf("LEADER SUPPLY <%d> OK\n",row[3]);									
					MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);
				}else if(leader!=rank){
					if(stock<150) {
						row[2]=150-stock;
						MPI_Send(&row, 1, MESSAGE_ARRAY,leader, SUPPLY_REQUEST, MPI_COMM_WORLD);
						MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE,MPI_ANY_TAG , MPI_COMM_WORLD, &status);
						assert(status.MPI_TAG==ACK );
						assert(status.MPI_SOURCE==leader );
						stock+=row[4];
					}
					MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, EXTERNAL_SUPPLY, MPI_COMM_WORLD);

				} else if(leader==rank){

					row[3]=row[1];
					if(stock<150) {
						row[1]-=(150-stock);
						stock=150;
					}
					MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, EXTERNAL_SUPPLY, MPI_COMM_WORLD);

				}
			}else if(status.MPI_TAG==SUPPLY_REQUEST){
				assert(rank==leader);
				if (row[1]>=row[2]) {
					row[4]=row[2];
					row[1]-=row[2];

				}else {
					row[4]=row[1];
					row[1]=0;
				}
				MPI_Send(&row, 1, MESSAGE_ARRAY,status.MPI_SOURCE, ACK, MPI_COMM_WORLD);

			}else if(status.MPI_TAG==PRINT){		
				if (leader==rank&&status.MPI_SOURCE==right_server_rank){
					
					MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);
					
				}else{
					printf("SERVER <%d> HAS QUANTITY <%d>\n",rank,stock);
					/*
					for(int i=0;i<children_count;i++){
						printf("<%d>\n",children[i]);
					}
					*/
					
					MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, PRINT, MPI_COMM_WORLD);
				}

			}else if(status.MPI_TAG==REPORT){				
				//printf("<%d> RECEIVED REPORT\n",rank);
				if (rank==leader&&status.MPI_SOURCE==right_server_rank){				
					printf("LEADER REPORT <%d> <%d>\n",rank,row[2]);//row2 to sunoliko 
					
					MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);	
				}else{
					for (int i=0; i<children_count;i++){
						//printf("SEND REPORT TO  <%d>\n",children[i]);
						MPI_Send(&row, 1, MESSAGE_ARRAY,children[i], REPORT, MPI_COMM_WORLD);
						MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE,MPI_ANY_TAG , MPI_COMM_WORLD, &status);
						assert(status.MPI_TAG==ACK );
						assert(status.MPI_SOURCE==children[i]);
						//printf("RECEIVED REPORT FROM <%d>\n",children[i]);
						tree_received_stock+=row[3];
						row[3]=0;
					}
					printf("REPORT <%d> <%d>\n",rank,tree_received_stock);										
					row[2]+=tree_received_stock;					
					
					MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank,REPORT , MPI_COMM_WORLD);
				}
			}else if(status.MPI_TAG==START_LEADER_ELECTION&&has_started_leader_election){
				row[0]=PROBE;
				row[1]=rank;
				row[2]=0;
				row[3]=1;
				MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, PROBE, MPI_COMM_WORLD);
				MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, PROBE, MPI_COMM_WORLD);
				has_started_leader_election=1;
			}else if(status.MPI_TAG==START_LEADER_ELECTION&&has_started_leader_election&&leader==-1){
				row[0]=PROBE;
				row[1]=rank;
				row[2]=0;
				row[3]=1;
				MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, PROBE, MPI_COMM_WORLD);
				MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, PROBE, MPI_COMM_WORLD);
				has_started_leader_election=1;
			}else if(status.MPI_TAG==START_LEADER_ELECTION&&!has_started_leader_election){
				row[0]=PROBE;
				row[1]=rank;
				row[2]=0;
				row[3]=0;
				for (int i=0;i<NUM_SERVERS;i++)	row[3]++;
				MPI_Send(&row, 1, MESSAGE_ARRAY,0,9, MPI_COMM_WORLD);				
			}else if(status.MPI_TAG==PROBE){
				if(rank==row[1]){
					leader=rank;
					for (int i=0;i<NUM_SERVERS;i++){
						if (i==rank) continue;
						row[3]=leader;//steile se olous poios einai o leader sto row3
						MPI_Send(&row, 1, MESSAGE_ARRAY,i, PROBE, MPI_COMM_WORLD);
					}

				}else if(row[1]>rank&&row[3]<(int)pow((double)2,(double)row[2])){
					row[3]++;
					if (status.MPI_SOURCE==left_server_rank)
						MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, PROBE, MPI_COMM_WORLD);
					else 
						MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, PROBE, MPI_COMM_WORLD);
	
				}else if(row[1]>rank&&row[3]>=(int)pow((double)2,(double)row[2])){	
					if (status.MPI_SOURCE==left_server_rank)
						MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, REPLY, MPI_COMM_WORLD);
					else
						MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, REPLY, MPI_COMM_WORLD);
					
				}
			}else if(status.MPI_TAG==REPLY){
				if (status.MPI_SOURCE==left_server_rank) already_received_reply_from_left=1;
				if (status.MPI_SOURCE==right_server_rank) already_received_reply_from_right=1;

				if(rank!=row[1]){
					if (status.MPI_SOURCE==left_server_rank)
						MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, REPLY, MPI_COMM_WORLD);
					else
						MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, REPLY, MPI_COMM_WORLD);
				}else{
					if ((status.MPI_SOURCE==left_server_rank&&already_received_reply_from_right)
					||(status.MPI_SOURCE==right_server_rank&&already_received_reply_from_left)){
						row[0]=PROBE;
						row[1]=rank;
						row[2]=row[2]+1;
						row[3]=1;
						MPI_Send(&row, 1, MESSAGE_ARRAY,right_server_rank, PROBE, MPI_COMM_WORLD);
						MPI_Send(&row, 1, MESSAGE_ARRAY,left_server_rank, PROBE, MPI_COMM_WORLD);
						}
					
				}		
					
			}else if(status.MPI_TAG==LEADER_ELECTION_DONE){
				leader=row[3];
				MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);

			}else if(status.MPI_TAG==TERMINATE){
				MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);

			}
			
		}
	}else {
		// clients
		//printf("[rank: %d] Client started\n", rank);
		int received_stock=0;
		int parent=-1;
		int active_requests=0;
		int * children=malloc(sizeof(int));
		int children_count=0;
		while(1){
			array_copy(row,zero_array);
			MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if(status.MPI_TAG==CONNECT){
				assert(status.MPI_SOURCE==0);
				parent=row[2];
				MPI_Send(&row, 1, MESSAGE_ARRAY, parent, CHILD, MPI_COMM_WORLD);
				MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				assert(status.MPI_TAG==ACK);
				assert(status.MPI_SOURCE==parent);
				MPI_Send(&row, 1, MESSAGE_ARRAY, 0, ACK, MPI_COMM_WORLD);	
			}else if(status.MPI_TAG==CHILD){
				children[children_count++]=status.MPI_SOURCE;
				children=realloc(children,sizeof(int)*children_count+1);	
				MPI_Send(&row, 1, MESSAGE_ARRAY, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);	
			}else if(status.MPI_TAG==CLIENT){
				assert(status.MPI_SOURCE==0);
				leader=row[3];
				MPI_Send(&row, 1, MESSAGE_ARRAY, 0, ACK, MPI_COMM_WORLD);
			}else if(status.MPI_TAG==ORDER){
				//printf("orderr from %d to sent order to %d\n",status.MPI_SOURCE);	
				MPI_Send(&row, 1, MESSAGE_ARRAY, parent, ORDER, MPI_COMM_WORLD);
				if(status.MPI_SOURCE==0){
					MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
					assert(status.MPI_TAG==ACK);	
					received_stock+=row[2];
					printf("CLIENT <%d> SOLD <%d>\n",row[1],row[2]);
					
					MPI_Send(&row, 1, MESSAGE_ARRAY, 0, ACK, MPI_COMM_WORLD);
				}
			
			}else if(status.MPI_TAG==REPORT){
				/*
				printf("<%d> CHILDREN:",rank);
					for(int i=0;i<children_count;i++){
						printf("{%d}",children[i]);
					}				
				printf("\n<%d> RECEIVED REPORT\n",rank);
				*/
				row[3]+=received_stock;				
				for (int i=0; i<children_count;i++){
					//printf("SEND REPORT TO <%d>\n",children[i]);
					MPI_Send(&row, 1, MESSAGE_ARRAY,children[i], REPORT, MPI_COMM_WORLD);
					MPI_Recv(&row, 1, MESSAGE_ARRAY, MPI_ANY_SOURCE,MPI_ANY_TAG , MPI_COMM_WORLD, &status);
					assert(status.MPI_TAG==ACK );
					assert(status.MPI_SOURCE==children[i]);
					//printf("RECEIVED REPORT FROM <%d>\n",children[i]);

				}
				MPI_Send(&row, 1, MESSAGE_ARRAY,parent,ACK, MPI_COMM_WORLD);

			}else if(status.MPI_TAG==TERMINATE){
				MPI_Send(&row, 1, MESSAGE_ARRAY,0, ACK, MPI_COMM_WORLD);

			}
		}
	}
	MPI_Finalize();
}
