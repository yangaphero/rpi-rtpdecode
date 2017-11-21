#include <stdio.h>  
#include <stdlib.h> 

//------------------cache-start--------------------
    //定义函数结果状态码  
    #define OK 1  
    #define ERROR 0  
    #define TRUE 1  
    #define FALSE 0  
      
    //定义循环队列空间大小  
    #define QUEUESIZE 1000 
      typedef struct _frame 
    {  
        unsigned char *data;//存储队列元素  
        int len;//队列头指针  
        unsigned  long  pts;
        unsigned  long  start_pts;
        //unsigned  long  dts;
        int64_t timestamp;
        int64_t start_timestamp;
    }frame;  //frame
      

      
      
    //定义数据类型  
    typedef frame ElemType ;  
      
    //循环队列存储结构  
    typedef struct _CircleQueue  
    {  
        ElemType data[QUEUESIZE];//存储队列元素  
        int front;//队列头指针  
        int rear;//队列尾指针  
        int count;//队列元素个数  
    }CircleQueue;  
      
      
    int InitQueue(CircleQueue *queue)  ;     
    int IsQueueEmpty(CircleQueue *queue)  ;
    int IsQueueFull(CircleQueue *queue)  ;
    int EnQueue(CircleQueue *queue, ElemType e)  ;
    ElemType DeQueue(CircleQueue *queue)  ;
    ElemType GetHead(CircleQueue *queue)  ;
    int ClearQueue(CircleQueue *queue )  ;
    int GetLength(CircleQueue *queue)  ;

//---------------cache-end------------

