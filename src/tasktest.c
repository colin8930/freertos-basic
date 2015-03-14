#include "FreeRTOS.h"
#include "fio.h"
#include "clib.h"
#include "task.h"
#include "host.h"

typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextSizeBlock;       /*<< The next free block by allocation size. */
    struct A_BLOCK_LINK *pxNextAddrBlock;       /*<< The next free block by physical memory address. */
    size_t xBlockSize;                          /*<< The size of the free block. */
} xBlockLink;

extern xBlockLink xStartSize;
int prng(void) __attribute__((naked));
xTaskHandle xHandle = NULL;




void test_task(void *pvParameters)
{
    while(1);	
}
void tasktest_command(int n, char *argv[]){

    int handle;
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog.txt");

    handle = host_action(SYS_OPEN, "output/syslog.txt", 7);

    while (1) {
        unsigned short stacksize = prng() & 0xFF; //0~255
        fio_printf(1, "try to create task %d bytes\r\n", stacksize);
        if(xTaskCreate(test_task,
                (signed portCHAR *) "test",
                stacksize /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) break;
                fio_printf(1, "created successfully\r\n");

        
        signed char buf[1024];
        vTaskList(buf);
        char tmp[] ="\n\rName          State   Priority  Stack  Num\n\r";
        fio_printf(1, tmp);
        host_action(SYS_WRITE, handle, (void *)tmp, strlen((const char *)tmp));
        fio_printf(1, "*******************************************\n\r");
        fio_printf(1, "%s\r\n", buf + 2);
        host_action(SYS_WRITE, handle, (void *)buf, strlen((const char *)buf));


        xBlockLink *pxBlock;
        vTaskSuspendAll();
        {

            pxBlock = xStartSize.pxNextSizeBlock;
			
            while ( pxBlock->pxNextSizeBlock ) {
                int out =  ( int )pxBlock->xBlockSize;
                char *str = "";
                char tmp[10];
                sprintf( tmp, ( char * ) "\t%d\t", out );
                strcpy(str, tmp);
                fio_printf(1, str);
                host_action(SYS_WRITE, handle, (void *)tmp, strlen(tmp));

                pxBlock = pxBlock->pxNextSizeBlock;
            
            }
		
	    }
	    xTaskResumeAll();
    }
    
    host_action(SYS_CLOSE, handle);
    xTaskResumeAll();
}
    

