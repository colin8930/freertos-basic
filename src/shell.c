#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void tasktest_command(int, char**);
void new_command(int, char **);
void cd_command(int, char **);
void pwd_command(int, char **);
void _command(int, char **);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}
char pwd[20] = "/romfs/"; //current  directory

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(new, "create new task"),
	MKCL(tasktest, "create new task"),
	MKCL(cd, "change the current dir"),
	MKCL(pwd, "show the current working dir"),
	MKCL(, ""),
};



int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

 return count;
}

void ls_command(int n, char *argv[]){
    fio_printf(1,"\r\n"); 
    int dir;
    if(n == 1){
        dir = fs_opendir(pwd);
        if(dir == -2) fio_printf(1, "error\r\n");
        if(dir == -1) fio_printf(1, "error\r\n");
    }else if(n == 2){
        char path[20] = "";
        strcpy(path, pwd);		
        strcat(path, argv[1]) ;
        dir = fs_opendir(path);
        if(dir == -2) fio_printf(1, "error\r\n");
        if(dir == -1) fio_printf(1, "error\r\n");
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }
    
    (void) dir;
}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if( fd == -2 || fd == -1)
		return fd;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
    }
	
    fio_printf(1, "\r");

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

    int dump_status = filedump(argv[1]);
	if(dump_status == -1){
		fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    }else if(dump_status == -2){
		fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

    int dump_status = filedump(buf);
	if(dump_status < 0)
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i = 0;i < sizeof(cl)/sizeof(cl[0]) - 1; ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;

    fio_printf(1, "\r\n");
    
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

void blank_task(void *pvParameters)
{
	while(1);	
}

void new_command(int n, char *argv[]){
	
    int new = 0;
    fio_printf(1, "\r\n");
    while(*(argv[1]++)) new=new*10+(*(argv[1]-1))-48;

    for(int i = 0; i < new; i++){
        if(xTaskCreate(blank_task,
                    (signed portCHAR *) "blank",
                    512 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
                        char str[3];
                        sprintf(str, "%d", i);
                        fio_printf(1, str);
                        fio_printf(1, " tasks are created successfully\r\n");
                        sprintf(str, "%d", new - i);
                        fio_printf(1, str);
                        fio_printf(1, " task(s) can't not be created \r\n");
                        break;
                    }
    }
    fio_printf(1, "\r\n");
}

void _command(int n, char *argv[]){
    (void)n; (void)argv;
    fio_printf(1, "\r\n");
}

void pwd_command(int n, char *argv[]){

    if(n == 1 ) {
        fio_printf(1, pwd);
        fio_printf(1, "/r/n");
    }
    else fio_printf(1, "Too many argument!\r\n");

}

void cd_command(int n, char *argv[]){
	
    int dir;
    if(n == 1) strcpy(pwd, "/romfs");  //go to home
    else if(n == 2){
        if(strcmp(argv[1], ".." ) == 0){			
            if(strcmp(pwd, "/romfs/") == 0) ;//do nothings
            else {
                int len = strlen(pwd) - 2; //find the lash slash				
                while(pwd[len--] != '/' );
                char tmp[30] = "";
                memcpy(tmp, pwd, len+2);		
                strcpy(pwd, tmp);			
                }
        }	else{
                char path[20] = "/romfs/";
                strcat(path, argv[1]) ;
                dir = fs_checkdir(path);
                if(dir == -2 || dir == -1) fio_printf(1, "error\r\n");
                else strcpy(pwd, path);
        }
    }
    else fio_printf(1, "Too many argument!\r\n");

    fio_printf(1, "\r\n");
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}
