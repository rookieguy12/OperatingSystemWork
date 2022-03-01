#include<sys/types.h>
#include<iostream>
#include<pthread.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>

using namespace std;
#define N 10  //缓冲区个数
#define SIZE_OF_BUFFER 20//缓冲区大小
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
    sem.sem_flg = 0;	//操作标记：0或IPC_NOWAIT等	
    semop(semid,&sem,1);	//1:表示执行命令的个数
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
	int shmid;
	int semid;
	pid_t readbuffer,writebuffer;
	union semun arg;
	shmid = shmget((key_t)0xF22,sizeof(ShareMemory),0666|IPC_CREAT);  //创建共享内存
	if(shmid==-1){
		printf("shmget error!\n");
		exit(1);
	}

	ShareMemory* mainShareMemory = nullptr;
	mainShareMemory = static_cast<ShareMemory*>(shmat(shmid,0,0)); //启动主进程main.cpp对共享内存的访问
	if(mainShareMemory==nullptr){
		printf("main.cpp shmat error!\n");
		exit(1);
	}
	mainShareMemory->end = mainShareMemory->start = 0;  			//初始化
	mainShareMemory->totalMessageNum = UINT32_MAX;

	semid = semget((key_t)0XF35,2,IPC_CREAT|0666);//使用2个信号量
	arg.val = N;
	if(semctl(semid,0,SETVAL,arg)<0)			//0号：空闲buffer个数
	{
		cout << "semctl 1 error!\n";
		exit(0);
	}
	arg.val = 0;
	if(semctl(semid,1,SETVAL,arg)<0)			//1号：满buffer个数
	{
		cout << "semctl 2 error!\n";
		exit(0);
	}

	if((readbuffer=fork())==0){					//创建readbuffer进程
		cout << "Create ReadBuffer process!\n";
		execl("./readfile","readfile",NULL);
	}else if((writebuffer=fork())==0){			//创建writebuffer进程
		cout << "Create WriteBuffer process!\n";
		execl("./writefile","writefile",NULL);
	}else{
		wait(NULL);
		wait(NULL);

		semctl(semid,3,IPC_RMID,arg);			//删除信号灯
		shmctl(shmid,IPC_RMID,NULL);			//删除共享内存
		cout << "Parent Process is over!\n";
	}
}
