#include<sys/types.h>
#include<iostream>
#include<pthread.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fstream>
#define N 10
#define SIZE_OF_BUFFER 20
using namespace std;
union semun {
    int val;
    struct semid_ds *buf; 
    unsigned short *array; 
    struct seminfo *__buf; 
};

struct ShareMemory{
	int start;  //read,start指向待读的
	int end;    //write,end指向待写区
	char buffer[N][SIZE_OF_BUFFER];
	size_t totalMessageNum;
};

void P(int semid,int index)
{	  
	struct sembuf sem;	
    sem.sem_num = index;
    sem.sem_op = -1;	
    sem.sem_flg = 0;	
    semop(semid,&sem,1);
    return;
}
void V(int semid,int index)
{	 
	struct sembuf sem;	
    sem.sem_num = index;
    sem.sem_op =  1;
    sem.sem_flg = 0;	
    semop(semid,&sem,1);	
    return;
}

int main(){
	const char * inputFile = "file1";
	int semid = semid = semget((key_t)0xF35,2,IPC_CREAT|0666);					  //获取信号量
	if(semid == -1){
		cout << "writefile.cpp semget error!\n";
		exit(2);
	}

	int shmid = shmget((key_t)0xF22,sizeof(struct ShareMemory),0666|IPC_CREAT);  //获取共享内存
	if(shmid == -1){
		printf("shmget error!\n");
		exit(2);
	}

	ShareMemory* writeAddress = NULL;
	writeAddress = static_cast<ShareMemory*>(shmat(shmid,0,0));

	ifstream inputFileStream(inputFile, ios::in | ios::binary);
	if (!inputFileStream.is_open())
	{
		cout << "readfile.cpp : cannot open inputFileStream!\n";
		exit(2);
	}
	inputFileStream.seekg(0,inputFileStream.end);
	size_t length = inputFileStream.tellg();
	writeAddress->totalMessageNum = length;
	inputFileStream.seekg(0, inputFileStream.beg);
	size_t sendNum = 0;
	char c;
	while(sendNum < length)
	{
		P(semid, 0);
		size_t rest = length - sendNum;
		if (rest < SIZE_OF_BUFFER)
		{
			inputFileStream.read(writeAddress->buffer[writeAddress->end],rest);
			sendNum += rest;
		}
		else
		{
			inputFileStream.read(writeAddress->buffer[writeAddress->end],SIZE_OF_BUFFER);
			sendNum += SIZE_OF_BUFFER;
		}
		writeAddress->end=(writeAddress->end+1)%N;
		V(semid, 1);
	}
	inputFileStream.close();
	cout << "Totally read " << sendNum << " bytes! Successifully write file!\n";
	return 0;
}
