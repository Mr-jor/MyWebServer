#include "threadpool.h"

/* 创建线程池 */
threadpool_t *threadpool_create(int thread_count, int queue_size, int flags) 
{
	threadpool_t *pool ;
	int i ;
	do
	{
		if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <=0 || queue_size > MAX_QUEUE){
			return NULL ;
		}

		if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL){
			break ;
		}

		/* 线程池属性初始化 */
		pool->thread_count = 0 ;
		pool->queue_size = queue_size ;
		pool->head = pool->tail = pool->count = 0 ;
		pool->shutdown = pool->started = 0 ;

		/* 分配线程队列和任务队列相关资源 */
		pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count) ;
		pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size) ;

		/* 初始化互斥锁和条件变量 */
		if((pthread_mutex_init(&(pool->lock),NULL) != 0 ) ||
			(pthread_cond_init(&(pool->notify),NULL) != 0 ) ||
			(pool->threads == NULL ) ||
			(pool->queue == NULL ))
		{
			break ;
		}

		/* 开始创建工作线程 */
		for(i = 0 ; i < thread_count ; ++i){
			if(pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void *)pool) != 0){
				threadpool_destroy(pool,0) ;
				return NULL ;
			}
			pool->thread_count++ ;
			pool->started++ ;
		}

		return pool ;

	} while (false);

	if(pool != NULL){
		threadpool_free(pool) ;
	}

	return NULL ;

}
/* 向线程池的任务队列中加入任务 */
int threadpool_add(threadpool_t *pool, void (*function)(void *) ,void *argument, int flags) 
{
	//printf("add to thread pool !\n") ;
	int err = 0 ;
	int next ;
	if(pool == NULL || function == NULL){
		return THREADPOOL_INVALID ;
	}
	if(pthread_mutex_lock(&(pool->lock),NULL) != 0){
		return THREADPOOL_LOCK_FAILURE ;
	}
	next = (pool->tail + 1) % pool->queue_size ;

	do
	{
		if(pool->count == pool->queue_size){
			err = THREADPOOL_QUEUE_FULL ;
			break ;
		}	
		if(pool->shutdown){
			err = THREADPOOL_SHUTDOWN ; 
			break ;
		}

		/* 向任务队列添加新任务 */
		pool->queue[pool->tail].function = function ;
		pool->queue[pool->tail].argument = argument ;
		pool->tail = next ;
		pool->count += 1 ;

		/* 向空闲并阻塞的工作线程发送消息 */
		if(pthread_cond_signal(&(pool->notify)) != 0){
			err = THREADPOOL_LOCK_FAILURE ;
			break ;
		}
	} while (false);

	if(pthread_mutex_unlock(&(pool->lock)) != 0)
		err = THREADPOOL_LOCK_FAILURE ;

	return err ;
}
/* 当线程池不再使用时，使所有工作线程退出运行函数 */
int threadpool_destroy(threadpool_t *pool, int flags) 
{
	printf("Thread pool destroy !\n") ;
	int i,err = 0 ;

	if(pool == NULL)
		return THREADPOOL_INVALID ;

	if(pthread_mutex_lock(&(pool->lock)) != 0)
		return THREADPOOL_LOCK_FAILURE ;

	do
	{
		if(pool->shutdown){
			err = THREADPOOL_SHUTDOWN ;
			break ;
		}

		pool->shutdown = (flags & THREADPOOL_GRACEFUL) ?
			graceful_shutdown : immediate_shutdown ;

		/* 唤醒所用阻塞的工作线程 */
		if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
			(pthread_mutex_unlock(&(pool->lock)) != 0))
		{
			err = THREADPOOL_LOCK_FAILURE ;
			break ;
		}

		/* 等待所用工作线程运行结束 */
		for(i = 0 ; i < pool->thread_count ; ++i)
		{
			if(pthread_join(pool->threads[i],NULL) != 0)
				err = THREADPOOL_THREAD_FAILURE ;
		}
	} while (false);

	if(!err)
	{
		threadpool_free(pool) ;
	}

	return err ;
}
/*  销毁线程池,释放线程池资源 */
int threadpool_free(threadpool_t *pool) 
{
	if(pool == NULL || pool->started > 0)
		return -1 ;

	if(pool->threads)
	{
		/* 释放线程标识符资源 和任务队列资源 */
		free(pool->threads) ;
		free(pool->queue) ;

		/* 释放互斥锁和条件变量资源 */
		pthread_mutex_lock(&(pool->lock)) ;
		pthread_mutex_destroy(&(pool->lock)) ;
		pthread_cond_destroy(&(pool->notify)) ;
	}

	free(pool) ;

	return 0 ;
}
/* 创建线程池中线程时，每个线程中运行的函数 （可重入的函数） */
static void *threadpool_thread(void *threadpool) 
{
	threadpool_t *pool = (threadpool_t *)threadpool ;
	threadpool_task_t task ;

	for(;;)
	{
		pthread_mutex_lock(&(pool->lock)) ;
		
		while( (pool->count == 0) && (!pool->shutdown)){
			pthread_cond_wait(&(pool->lock),&(pool->notify)) ;
		}
		
		if( (pool->shutdown == immediate_shutdown) ||
			((pool->shutdown == graceful_shutdown) && 
			(pool->count == 0)))
		{
			break ;
		}

		/* 将任务队列中任务 分配给空闲的工作线程 */

		task.function = pool->queue[pool->head].function ;
		task.argument = pool->queue[pool->head].argument ;
		pool->head = (pool->head + 1) % pool->queue_size ;
		pool->count-- ;

		pthread_mutex_unlock(&(pool->lock)) ;

		/* 执行工作线程中任务 */
		(*(task.function))(task.argument) ;
	}
	--pool->started ;

	pthread_mutex_unlock(&(pool->lock)) ;
	pthread_exit(NULL) ;

	return NULL ;
}
