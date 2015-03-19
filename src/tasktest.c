#include "FreeRTOS.h"
#include "fio.h"
#include "clib.h"
#include "task.h"
#include "host.h"

typedef struct A_BLOCK_LINK
{
    /* for heap_ww*/
    struct A_BLOCK_LINK *pxNextSizeBlock;      
    struct A_BLOCK_LINK *pxNextAddrBlock;     
    
    
    /* for heap_2
    struct A_BLOCK_LINK *pxNextFreeBlock;      */

    size_t xBlockSize;                          /*<< The size of the free block. */
} xBlockLink;


int prng(void) __attribute__((naked));
xTaskHandle xHandle = NULL;
extern xBlockLink xStartSizeGet( void );
extern xBlockLink xEndGet( void );
extern xTaskHandle xcmdHandle;
extern xTaskHandle xTaskGetIdleTaskHandle( void );

void test_task(void *pvParameters)
{
    while(1);	
}
void tasktest_command(int n, char *argv[]){
    static xBlockLink xstartSize;
    static xBlockLink xend;

	
    xTaskHandle xIHandle;
    xIHandle = xTaskGetIdleTaskHandle();
	
    int handle;
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog.txt");
    handle = host_action(SYS_OPEN, "output/syslog.txt", 7);
    while ( 1 ) {
        xHandle=NULL;
		
        vTaskPrioritySet(xIHandle, 0);
        unsigned short stacksize = prng() & 0x7F; //0~255
        if(stacksize < 48 ) stacksize+=48;
        fio_printf(1, "\n\r try to create task %d bytes\r\n", stacksize);
        if( xTaskCreate(test_task,
                (signed portCHAR *) "test",
                stacksize /* stack size */, NULL, tskIDLE_PRIORITY + 1, &xHandle) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) break;
 
        fio_printf(1, "created successfully\r\n");        

        char tmp[80];
        sprintf( tmp, ( char * ) "\r\n\r\n\r\ncreate a %d bytes task \n\rName          State   Priority  Stack  Num", stacksize*4);
        fio_printf(1, tmp);
        host_action(SYS_WRITE, handle, (void *)tmp, strlen((const char *)tmp));
        signed char buf[1024];
        vTaskList(buf);
        fio_printf(1, "*******************************************\n\r");
        fio_printf(1, "%s\r\n", buf + 2);
        host_action(SYS_WRITE, handle, (void *)buf, strlen((const char *)buf));


        static xBlockLink *pxBlock;
        if( stacksize % 3 == 1 ||stacksize % 3 == 2){
            host_action(SYS_WRITE, handle, (void *)"\r\nthis task will be deleted, then the memory blocks will be: \n\r", strlen("this task will be deleted, then the memory blocks will be: \n\r"));
        }
        else host_action(SYS_WRITE, handle, (void *)"the memory blocks will be: \n\r", strlen("the memory blocks will be: \n\r"));
        vTaskSuspendAll();
        {
            xstartSize = xStartSizeGet();
			/* for heap_2
            pxBlock = xstartSize.pxNextFreeBlock;*/
            /* for heap_ww*/
            pxBlock = xstartSize.pxNextSizeBlock;
            int i = 0;
            xend = xEndGet();
            while ( pxBlock->pxNextSizeBlock) {
				
                char tmp[50];
                sprintf( tmp, ( char * ) " block %d: %d ", i, (int) pxBlock->xBlockSize );
                fio_printf(1, tmp);
                host_action(SYS_WRITE, handle, (void *)tmp, strlen(tmp));
                i++;
                /* for heap_2
                pxBlock = pxBlock->pxNextFreeBlock;*/
                /* for heap_ww*/
                pxBlock = pxBlock->pxNextSizeBlock;
            
            }
		
	    }
	    
	   
    
	    
	    if( stacksize % 3 == 1 ||stacksize % 3 == 2){
            vTaskDelete( xHandle );
            vTaskPrioritySet( xIHandle, 2 );
            vTaskDelay( 3 );
        }
        xTaskResumeAll();
    }
    
    vTaskResume(xcmdHandle);
    host_action(SYS_CLOSE, handle);
    
}
    


