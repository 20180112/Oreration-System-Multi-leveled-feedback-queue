#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <string.h>
#define totalprocess 15


HANDLE sema1,sema2;//sema1为互斥信号，sema2为同步信号
typedef struct PCB{
    int pid;
    char state;//本实验也可以用state表示PCB是否被调用过
    int priority;
    int neededTime_const;
    int neededTime;
    int arrivetime;
    int totalWaitTime;
    struct PCB *next;//表示此进程完成后下一个进行的进程
};
typedef struct PCB_lst
{
    struct PCB Pnow;
    struct PCB_lst* Pnext;
};
struct PCB_lst* Lst1;
struct PCB_lst* Lst2;
struct PCB_lst* Lst3;
struct PCB_lst* Lst4;
struct PCB_lst* Lst5;

HANDLE genPCB;

int nextarrivetime = 0;//记录下一次进程进入时间
int numproc = 0;//记录进程被创建的个数
int finishproc = 0;//记录进程被完成的个数
int rightnowtime = 0;//记录此时的时间
int number = 0;//增加pid用于赋值
int time_piece[5] = {10,20,40,80,160};

DWORD tid1; 
int Scheduler();
DWORD generator();
int executor(struct PCB *PCBexe,int lstnumber);

int main(){
    //给全局变量五个链表初始化，分配空间
    Lst1 = malloc(sizeof(struct PCB_lst));
    Lst2 = malloc(sizeof(struct PCB_lst));
    Lst3 = malloc(sizeof(struct PCB_lst));
    Lst4 = malloc(sizeof(struct PCB_lst));
    Lst5 = malloc(sizeof(struct PCB_lst));
    Lst1->Pnext = NULL;
    Lst2->Pnext = NULL;
    Lst3->Pnext = NULL;
    Lst4->Pnext = NULL;
    Lst5->Pnext = NULL;
    //设置同步和互斥信号量
    sema1 = CreateSemaphore(NULL,1,1,NULL);
    sema2 = CreateSemaphore(NULL,0,1,NULL);
    //创建线程
    genPCB = CreateThread(NULL,0,generator,NULL,0,&tid1);
   
    Scheduler();
    
    WaitForSingleObject(genPCB,INFINITE);
    //关闭信号量
    CloseHandle(sema1);
	CloseHandle(sema2);
    //关闭线程
    CloseHandle(genPCB);
    printf("please input any key....");
    getchar();
    return 1;
}
DWORD generator(){
    //随机化rand函数
    srand(time(0));
    for (int i = 0; i < totalprocess; i++)
    {
        //创建线程
        struct PCB* Pnew = malloc(sizeof(struct PCB));//生成新的一个线程，配置线程的空间
        number ++;Pnew->pid = number;
        Pnew->totalWaitTime = 0;
        Pnew->neededTime = rand()%198+2;
        Pnew->arrivetime = nextarrivetime;
        Pnew->neededTime_const = Pnew->neededTime;//保持固定的所需时间
        printf("Generator:process%d has been generated.it's arrive time is %d ms,it's neededtime is %d ms.It's put in line1,priority is 1.\n",Pnew->pid,Pnew->arrivetime,Pnew->neededTime);

        WaitForSingleObject(sema1,INFINITE);      
        if (Lst1->Pnext == NULL)//第一个队列中无数据
        {//队列中存在头结点
            Lst1->Pnext = malloc(sizeof(struct PCB_lst));
            Lst1->Pnext->Pnow = *Pnew;
            Lst1->Pnext->Pnext = NULL;
        }
        else{//若线程列表中已经存在线程，则将线程创建在列表的最后，满足先进先出
            struct PCB_lst *p1 = Lst1;
            while (p1->Pnext != NULL)
            {
                p1 = p1->Pnext;
            }
            //分配空间
            p1->Pnext = malloc(sizeof(struct PCB_lst));
            p1->Pnext->Pnow = *Pnew;
            p1->Pnext->Pnext = NULL;
        }       
        //释放同步和互斥信号
        ReleaseSemaphore(sema1,1, NULL);
        ReleaseSemaphore(sema2,1, NULL);
        numproc ++;
        int waittime = rand()%99 + 1;//设置生成器的睡眠时间
        printf("Generator:wait for %d(ms) to generate next process....\n",waittime);
        Sleep(waittime);//控制在毫秒内有新的进程产生
        nextarrivetime = nextarrivetime + waittime;
    }
    return 1;
}
int executor(struct PCB *PCBexe,int lstnumber){//lstnumber表示处理的列表
    if (lstnumber == 1)
    {//处理第一个列表中的PCB
        WaitForSingleObject(sema1,INFINITE);
        if (PCBexe->neededTime - time_piece[0] > 0)//判断线程能否直接在此次完成
        {//不能完成
            PCBexe->neededTime = PCBexe->neededTime - time_piece[0];//将此线程的所需时间减小
            rightnowtime = rightnowtime + 10;
            if (Lst2->Pnext == NULL)//第一个队列中无数据
            {//队列中存在头结点
                Lst2->Pnext = malloc(sizeof(struct PCB_lst));
                Lst2->Pnext->Pnow = *PCBexe;
                Lst2->Pnext->Pnext = NULL;
            }
            else{
                struct PCB_lst *p2 = Lst2;
                while (p2->Pnext != NULL)
                {
                    p2 = p2->Pnext;
                }
                p2->Pnext = malloc(sizeof(struct PCB_lst));
                p2->Pnext->Pnow = *PCBexe;
                p2->Pnext->Pnext = NULL;
            } 
            Sleep(time_piece[0]);//毫秒时间睡眠表示进程正在被执行
            printf("Executor:process%d's executed %d(ms) in line1.it still need %d ms\n",PCBexe->pid,time_piece[0],PCBexe->neededTime);
            printf("Scheduler:process%d has been put on the line2,priority is 2.\n",PCBexe->pid);
        }
        else{
            Sleep(PCBexe->neededTime);
            rightnowtime = rightnowtime + PCBexe->neededTime;
            PCBexe->totalWaitTime = rightnowtime - PCBexe->arrivetime - PCBexe->neededTime_const;
            printf("Executor:process%d in list1 have been excuted!Total wait time is %d(ms)\n",PCBexe->pid,PCBexe->totalWaitTime);
            finishproc++;
        }
        ReleaseSemaphore(sema1,1, NULL);
    }
    else if (lstnumber == 2)
    {
        
        if (PCBexe->neededTime - time_piece[1] > 0)
        {   
            PCBexe->neededTime = PCBexe->neededTime-time_piece[1];
            rightnowtime = rightnowtime + 20;
            if (Lst3->Pnext == NULL)//第一个队列中无数据
            {//队列中存在头结点
                Lst3->Pnext = malloc(sizeof(struct PCB_lst));
                Lst3->Pnext->Pnow = *PCBexe;
                Lst3->Pnext->Pnext = NULL;
            }
            else{
                struct PCB_lst *p3 = Lst3;
                while (p3->Pnext != NULL)
                {
                    p3 = p3->Pnext;
                }
                p3->Pnext = malloc(sizeof(struct PCB_lst));
                p3->Pnext->Pnow = *PCBexe;
                p3->Pnext->Pnext = NULL;
            } 
            Sleep(time_piece[1]);
            printf("Executor:process%d's executed %d(ms) in line2.it still need %d ms\n",PCBexe->pid,time_piece[1],PCBexe->neededTime);
            printf("Scheduler:process%d has been put on the line3,priority is 3.\n",PCBexe->pid);
        }
        else{
            Sleep(PCBexe->neededTime);
            rightnowtime = rightnowtime + PCBexe->neededTime;
            PCBexe->totalWaitTime = rightnowtime - PCBexe->arrivetime - PCBexe->neededTime_const;
            printf("Executor:process%d in list2 have been excuted!Total wait time is %d(ms)\n",PCBexe->pid,PCBexe->totalWaitTime);
            finishproc++;
        }
    }    
    else if (lstnumber == 3)
    {
        if (PCBexe->neededTime - time_piece[2] > 0)
        {   
            PCBexe->neededTime = PCBexe->neededTime-time_piece[2];
            rightnowtime = rightnowtime + 40;
            if (Lst4->Pnext == NULL)//第一个队列中无数据
            {//队列中存在头结点
                Lst4->Pnext = malloc(sizeof(struct PCB_lst));
                Lst4->Pnext->Pnow = *PCBexe;
                Lst4->Pnext->Pnext = NULL;
            }
            else{
                struct PCB_lst *p4 = Lst4;
                while (p4->Pnext != NULL)
                {
                    p4 = p4->Pnext;
                }
                p4->Pnext = malloc(sizeof(struct PCB_lst));
                p4->Pnext->Pnow = *PCBexe;
                p4->Pnext->Pnext = NULL;
            }
            Sleep(time_piece[2]);
            printf("Executor:process%d's executed %d(ms) in line3.it still need %d ms\n",PCBexe->pid,time_piece[2],PCBexe->neededTime);
            printf("Scheduler:process%d has been put on the line4,priority is 4.\n",PCBexe->pid);
        }
        else{
            Sleep(PCBexe->neededTime);
            rightnowtime = rightnowtime + PCBexe->neededTime;
            PCBexe->totalWaitTime = rightnowtime - PCBexe->arrivetime - PCBexe->neededTime_const;
            printf("Executor:process%d in list3 have been excuted!Total wait time is %d(ms)\n",PCBexe->pid,PCBexe->totalWaitTime);
            finishproc++;
        }
    }       
    else if (lstnumber == 4)
    {
        if (PCBexe->neededTime - time_piece[3] > 0)
        {   
            PCBexe->neededTime = PCBexe->neededTime-time_piece[3];
            rightnowtime = rightnowtime + 80;
            if (Lst5->Pnext == NULL)//第一个队列中无数据
            {//队列中存在头结点
                Lst5->Pnext = malloc(sizeof(struct PCB_lst));
                Lst5->Pnext->Pnow = *PCBexe;
                Lst5->Pnext->Pnext = NULL;
            }
            else{
                struct PCB_lst *p5 = Lst5;
                while (p5->Pnext != NULL)
                {
                    p5 = p5->Pnext;
                }
                p5->Pnext = malloc(sizeof(struct PCB_lst));
                p5->Pnext->Pnow = *PCBexe;
                p5->Pnext->Pnext = NULL;
            } 
            Sleep(time_piece[3]);
            printf("Executor:process%d's executed %d(ms) in line4.it still need %d ms\n",PCBexe->pid,time_piece[3],PCBexe->neededTime);
            printf("Scheduler:process%d has been put on the line5,priority is 5.\n",PCBexe->pid);
        }
        else{
            Sleep(PCBexe->neededTime);
            rightnowtime = rightnowtime + PCBexe->neededTime;
            PCBexe->totalWaitTime = rightnowtime - PCBexe->arrivetime - PCBexe->neededTime_const;
            printf("Executor:process%d in list4 have been excuted!Total wait time is %d(ms)\n",PCBexe->pid,PCBexe->totalWaitTime);
            finishproc++;
        }
    }
    else
    {
        if (PCBexe->neededTime - time_piece[4] > 0)
        {   
            PCBexe->neededTime = PCBexe->neededTime-time_piece[4];
            rightnowtime = rightnowtime + 160;
            if (Lst5->Pnext == NULL)//第一个队列中无数据
            {//队列中存在头结点
                Lst5->Pnext = malloc(sizeof(struct PCB_lst));
                Lst5->Pnext->Pnow = *PCBexe;
                Lst5->Pnext->Pnext = NULL;
            }
            else{
                struct PCB_lst *p5 = Lst5;
                while (p5->Pnext != NULL)
                {
                    p5 = p5->Pnext;
                }
                p5->Pnext = malloc(sizeof(struct PCB_lst));
                p5->Pnext->Pnow = *PCBexe;
                p5->Pnext->Pnext = NULL;
            } 
            Sleep(time_piece[4]);
            printf("Executor:process%d's executed %d(ms) in line5.it still need %d ms\n",PCBexe->pid,time_piece[4],PCBexe->neededTime);
            printf("Scheduler:process%d has been put on the line5,priority is 5.\n",PCBexe->pid);
        }
        else{
            Sleep(PCBexe->neededTime);
            rightnowtime = rightnowtime + PCBexe->neededTime;
            PCBexe->totalWaitTime = rightnowtime - PCBexe->arrivetime - PCBexe->neededTime_const;
            printf("Executor:process%d in list5 have been excuted!Total wait time is %d(ms)\n",PCBexe->pid,PCBexe->totalWaitTime);
            finishproc++;
        }
    }    
    return 1;          
}
int Scheduler(){
    struct PCB_lst* p_lst1 = Lst1;
    struct PCB_lst* p_lst2 = Lst2;
    struct PCB_lst* p_lst3 = Lst3;
    struct PCB_lst* p_lst4 = Lst4;
    struct PCB_lst* p_lst5 = Lst5;
    while (1)
    {
        if (finishproc == totalprocess)
        {
            break;
        }
        else if (finishproc == numproc)
        {//当生成的进程都被处理完时
            WaitForSingleObject(sema2,INFINITE);
        }
        while (p_lst1->Pnext != NULL)
        {
            //无需考虑优先级，在执行过程中不存在添加线程的动作，因为存在互斥信号
            executor(&(p_lst1->Pnext->Pnow),1);
            p_lst1->Pnext = p_lst1->Pnext->Pnext;
        }
        while (p_lst2->Pnext != NULL)
        {
            while (p_lst1->Pnext != NULL)
            {//每次查询链表2有无数据时，要考虑突然添加进队列1的线程
                executor(&(p_lst1->Pnext->Pnow),1);
                p_lst1->Pnext = p_lst1->Pnext->Pnext;
            }
            //若无，则直接执行队列2中的线程
            executor(&(p_lst2->Pnext->Pnow),2);
            p_lst2->Pnext = p_lst2->Pnext->Pnext;
        }    
        while (p_lst3->Pnext != NULL)
        {//对队列3进行操作时，要考虑突然加入队列1或队列2的线程，它们的优先级更高
            while (p_lst1->Pnext != NULL)
            {   
                executor(&(p_lst1->Pnext->Pnow),1);
                p_lst1->Pnext = p_lst1->Pnext->Pnext;

            }                
            while (p_lst2->Pnext != NULL)
            {
                executor(&(p_lst2->Pnext->Pnow),2);
                p_lst2->Pnext = p_lst2->Pnext->Pnext;
            }
            //若无，则直接执行队列3中的线程
            executor(&(p_lst3->Pnext->Pnow),3);
            p_lst3->Pnext = p_lst3->Pnext->Pnext;
        }   
        while (p_lst4->Pnext != NULL)
        {//对队列4进行操作时，要考虑突然加入队列1或队列2或队列3的线程，它们的优先级更高
            while (p_lst1->Pnext != NULL)
            {          
                executor(&(p_lst1->Pnext->Pnow),1);
                p_lst1->Pnext = p_lst1->Pnext->Pnext;
            }    
            while (p_lst2->Pnext != NULL)
            {
                executor(&(p_lst2->Pnext->Pnow),2);
                p_lst2->Pnext = p_lst2->Pnext->Pnext;
            }
            while (p_lst3->Pnext != NULL)
            {
                executor(&(p_lst3->Pnext->Pnow),3);
                p_lst3->Pnext = p_lst3->Pnext->Pnext;
            }
            //若无，则直接执行队列4中的线程
            executor(&(p_lst4->Pnext->Pnow),4);
            p_lst4->Pnext = p_lst4->Pnext->Pnext;
        }
        while (p_lst5->Pnext != NULL)
        {//对队列5进行操作时，要考虑突然加入队列1或队列2或队列3或队列4的线程，它们的优先级更高
            while (p_lst1->Pnext != NULL)
            {                            
                executor(&(p_lst1->Pnext->Pnow),1);
                p_lst1->Pnext = p_lst1->Pnext->Pnext;
            }    
            while (p_lst2->Pnext != NULL)
            {
                executor(&(p_lst2->Pnext->Pnow),2);
                p_lst2->Pnext = p_lst2->Pnext->Pnext;    
            }     
            while (p_lst3->Pnext != NULL)
            {
                executor(&(p_lst3->Pnext->Pnow),3);
                p_lst3->Pnext = p_lst3->Pnext->Pnext;        
            }
            while (p_lst4->Pnext != NULL)
            {
                executor(&(p_lst4->Pnext->Pnow),4);
                p_lst4->Pnext = p_lst4->Pnext->Pnext;
            }
            //若无，则直接执行队列5中的线程
            executor(&(p_lst5->Pnext->Pnow),5);
            p_lst5->Pnext = p_lst5->Pnext->Pnext;
        }
    }
    printf("All processes have been executed!The all use time is %d ms.\n",rightnowtime);
    return 0;
}
