#ifndef THREADPOOL
#define THREADPOOL
#include <pthread.h>
#include "requestData.h"

/* 定义错误类型 */
const int THREADPOOL_INVALID = -1 ;
const int THREADPOOL_LOCK_FAILURE = -2 ;
const int THREADPOOL_QUEUE_FULL = -3 ;
const int THREADPOOL_SHUTDOWN = -4 ;
const int THREADPOOL_THREAD_FAILURE = -5 ;
const int THREADPOOL_GRACEFUL = 1 ;

/* 线程数属性参数 */
const int MAX_THREADS = 1024 ;
const int MAX_QUEUE = 65535 ;

/* 关闭线程池的方式类型 */
typedef enum 
{
	immediate_shutdown = 1 ,
	graceful_shutdown =2
} threadpool_shutdown_t;

/* 任务队列中任务的类型定义 */
typedef struct 
{
	void (*function)(void *);
	void *argument ;
} threadpool_task_t;

/* 线程池对象 */
struct threadpool_t
{
	pthread_mutex_t lock ;
	pthread_cond_t notify ;
	pthread_t *threads ;
	threadpool_task_t *queue ;
	int thread_count ;
	int queue_size ;
	int head ;
	int tail ;
	int count ;
	int shutdown ;
	int started ;
};

/* 创建线程池 */
threadpool_t *threadpool_create(int thread_count, int queue_size, int flags) ;
/* 向线程池的任务队列中加入任务 */
int threadpool_add(threadpool_t *pool, void (*function)(void *) ,void *argument, int flags) ;
/* 当线程池不再使用时，使所有工作线程退出运行函数 */
int threadpool_destroy(threadpool_t *pool, int flags) ;
/* 销毁线程池,释放线程池资源*/
int threadpool_free(threadpool_t *pool) ;
/* 创建线程池中线程时，每个线程中运行的函数 （可重入的函数） */
static void *threadpool_thread(void *threadpool) ;

#endif