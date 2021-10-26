#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

/* 잘못된 메모리 접근을 보호 */
void check(void* vaddr){
  struct thread* cur = thread_current();
	
  if(!is_user_vaddr(vaddr)){
    exit(-1);
  }	

  if(pagedir_get_page(cur->pagedir, vaddr) == NULL){
    exit(-1);
  }

}

/* halt */
void halt(void){
  shutdown_power_off();
}

/* exit */
void exit(int status){
  struct thread* cur = thread_current();

  cur->exitStatus = status;
  cur->exitFlag = 1;
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

/* execution */
tid_t exec(const char* file){
  return process_execute(file);
}

/* wait */
int wait(tid_t tid){
  return process_wait(tid);
}

/* simple read */
int read(int fd, void* buffer, int size){
  int i;
  char temp;
  if(fd == 0){	// STDIN
    for(i=0; i<size; i++){
	  temp = input_getc();
	  ((char*)buffer)[i]  = temp;
      if(temp == '\0'){
        break;
      }
    }

    if(i != size){	// error
      return -1;
    }

    return size;
  }

  return -1;
}

/* simple write */
int write(int fd, const void* buffer, unsigned size){
  
  if(fd == 1){
    putbuf(buffer, size);
    return size;
  }
  return -1;
}

/* return fibonacci value */
int fibonacci(int n)
{
	int i,fn=2, f1=1, f2=1;

	if(n==1 || n==2){
		return 1;
	}
	else{
		for(i=3; i<=n; i++)
		{
			fn=f1+f2;
			f2=f1;
			f1=fn;
		}
	}
	return fn;
}

/* return max of four int */
int max_of_four_int(int a, int b, int c, int d)
{
	int w1, w2;
	w1 = a>b ? a : b;
	w2 = c>d ? c : d;
	return w1>w2 ? w1 : w2;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	uint32_t callNum=*(uint32_t*)(f->esp);
	void *esp=f->esp;
	uint32_t returnValue=f->eax;
	int status,fd;
	uint32_t tid;
	unsigned size;
	char* file;
	void *buffer;

	if(callNum==SYS_WAIT)
	{
		check(esp+4);
		tid=*(uint32_t*)(esp+4);
		returnValue=(uint32_t)wait(tid);
	}
	else if(callNum==SYS_HALT){
		halt();
	}
	else if(callNum==SYS_EXIT)
	{
		check(esp+4);
		status=*(int*)(esp+4);
		exit(status);
	}
	else if(callNum==SYS_READ)
	{
		check(esp+4); check(esp+8); check(esp+12);
		fd=*(int*)(esp+4);
		buffer=(void*)(*(uint32_t*)(esp+8));
		size=*(unsigned*)(esp+12);
		returnValue=(uint32_t)read(fd,buffer, size);
	}
	else if(callNum==SYS_EXEC)
	{
		check(esp+4);
		file=(char *)(*(uint32_t*)(esp+4));
		returnValue=(uint32_t)exec(file);
	}
	else if(callNum==SYS_WRITE)
	{
		check(esp+4);check(esp+8);check(esp+12);
		fd=*(int*)(esp+4);
		buffer=(void*)(*(uint32_t*)(esp+8));
		size=*(unsigned*)(esp+12);
		returnValue=(uint32_t)write(fd, buffer, size);
	}
	else if(callNum==SYS_FIBONACCI)
	{
		check(esp+4);
		returnValue=(uint32_t)fibonacci(*(int*)(esp+4));
	}
	else if(callNum==SYS_MAX_OF_FOUR_INT)
	{
		check(esp+4); check(esp+8); check(esp+12); check(esp+16);
		int a=*(int*)(esp+4);
		int b=*(int*)(esp+8);
		int c=*(int*)(esp+12);
		int d=*(int*)(esp+16);
		returnValue=(uint32_t)max_of_four_int(a,b,c,d);
	}
	
	f->eax=returnValue;
}

