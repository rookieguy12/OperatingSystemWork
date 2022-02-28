#include<sys/types.h>
#include<iostream>
#include<pthread.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdio.h>
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
	const string outputImageName = "output";
	int semid = semget((key_t)0xF35,2,IPC_CREAT|0666);						//获取信号量
	if(semid ==-1){  					   
		cout << "readFile: cannot get semophore!\n";
		exit(2);
	}

	int shmid = shmget((key_t)0xF22,sizeof(ShareMemory),0666|IPC_CREAT);  	//获取共享内存
	if(shmid==-1){
		cout << "readFile: cannnot get shared memory!\n";
		exit(2);
	}

	ShareMemory* readAddress = nullptr;
	readAddress = static_cast<ShareMemory*>(shmat(shmid,0,0));				//得到共享内存

	ofstream outputStream;													//打开写入文件
	outputStream.open(outputImageName, ios::out|ios::binary);
	if(!outputStream.is_open()){
		cout << "outputFile cannot open!\n";
		exit(2);
	}

	int alreadyReadNum = 0;
	char readByte;
	while(alreadyReadNum < readAddress->totalMessageNum){
		P(semid,1); 														//从共享内存读取
		int rest = readAddress->totalMessageNum - alreadyReadNum;
		if (rest < SIZE_OF_BUFFER)
		{
			outputStream.write(readAddress->buffer[readAddress->start],rest);
			alreadyReadNum += rest;
		}
		else
		{
			outputStream.write(readAddress->buffer[readAddress->start], SIZE_OF_BUFFER);
			alreadyReadNum += SIZE_OF_BUFFER;
		}
		readAddress->start=(readAddress->start+1)%N;
		V(semid,0); 														//缓冲区空余位置数
		//cout << "wf: " << alreadyReadNum << " : " << readAddress->totalMessageNum << endl;
	}
	outputStream.close();
	cout << "Totally write " << alreadyReadNum << " bytes! Successifully write to outputfile!\n";
	return 0;
}
