#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "threads/synch.h"


static void syscall_handler (struct intr_frame *);

/* check invalid memory access */
void check(void* vaddr){
  struct thread* cur = thread_current();
	
  if(!is_user_vaddr(vaddr)){
    exit(-1);
  }	

  if(pagedir_get_page(cur->pagedir, vaddr) == NULL){
    exit(-1);
  }
}

/* check fd */
void checkFd(int fd){
	struct thread* cur = thread_current();
	if(cur->files[fd] == NULL) {
		exit(-1);
	}
}

/* close all fd */
void closeAllFd(){
	int i, fdCnt;
	struct thread* cur = thread_current();
	fdCnt = cur->fd;
	for(i=2; i<fdCnt; i++){
		if(cur->files[i] == NULL){
			continue;
		}else close(i);
	}
}

/* wait All child process */
void waitAll(){
	struct thread* cur = thread_current();
	struct thread* child;
	struct list* childList = &cur->child;
	struct list_elem* ptr = list_begin(childList);

	while( ptr != list_end(childList) ){
		child = list_entry(ptr, struct thread, current);
		process_wait(child->tid);
		ptr = list_next(ptr);
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
  closeAllFd();
  waitAll();
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
  int i, ret=-1;
  char temp;
  sema_down(&mutex);
  readCount++;
  if(readCount == 1){
	  sema_down(&wrt);
  }
  sema_up(&mutex);

  /* file read - critical section */
  if(fd == 0){	// STDIN
    for(i=0; i<size; i++){
	  temp = input_getc();
	  ((char*)buffer)[i]  = temp;
      if(temp == '\0'){
        break;
      }
    }

    if(i != size){	// error
      ret = -1;
    }else ret = size;

  }else{
	  struct thread* cur = thread_current();
	  ret = file_read(cur->files[fd], buffer, size);
  }

  sema_down(&mutex);
  readCount--;
  if(readCount == 0){
	  sema_up(&wrt);
  }
  sema_up(&mutex);
  
  return ret;
}

/* simple write */
int write(int fd, const void* buffer, unsigned size){
  int ret = -1;
  
  sema_down(&wrt);

  /* file write - critical section */
  if(fd == 1){
    putbuf(buffer, size);
    ret = size;
  }else{
	  struct thread* cur = thread_current();
	  ret = file_write(cur->files[fd], buffer, size);
  }

  sema_up(&wrt);

  return ret;
}

/* file open */
int open(const char* file){
	
	sema_down(&mutex);
	readCount++;
	if(readCount == 1){
		sema_down(&wrt);
	}
	sema_up(&mutex);

	int ret = -1;
	struct file* curFile = filesys_open(file);

	/*  file open - critiacl section*/
	if(curFile == NULL){
		ret = -1;
	}else{
		struct thread* cur = thread_current();
		cur->files[cur->fd] = curFile;
		if(strcmp(file, cur->name) == 0  ){	// avoid collision
			file_deny_write(cur->files[cur->fd]);
		}
		ret = cur->fd;
		cur->fd++;
	}

	sema_down(&mutex);
	readCount--;
	if(readCount == 0){
		sema_up(&wrt);
	}
	sema_up(&mutex);

	return ret;
}

/* return file size */
int getFileSize(int fd){
	struct thread*cur = thread_current();
	return file_length(cur->files[fd]);
}

/* close file */
void close(int fd){
	struct thread* cur = thread_current();
	file_close(cur->files[fd]);
	cur->files[fd] = NULL;
}

/* create file */
bool create(const char* file, unsigned initial_size){
	return filesys_create(file, initial_size);
}

/* remove file */
bool remove(const char* file){
	return filesys_remove(file);
}

/* seek file */
void seek(int fd, unsigned pos){
	struct thread* cur = thread_current();
	file_seek(cur->files[fd], pos);
}

/* tell file */
unsigned tell(int fd){
	struct thread * cur = thread_current();
	return file_tell(cur->files[fd]);
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
  sema_init(&mutex, 1);
  readCount = 0;
  sema_init(&wrt, 1);
  
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
		check(buffer);
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
		check(buffer);
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
	}else if(callNum == SYS_CREATE){
		check(esp+4); check(esp+8);
		char* file = *(char**)(esp+4);
		//if(file == NULL) exit(-1);
		if(file == NULL){
			exit(-1);
		}
		check(file);
		unsigned initial_size = *(unsigned*)(esp+8);
		returnValue = (uint32_t)create(file, initial_size);
	}else if(callNum == SYS_REMOVE){
		check(esp+4);
		char* file = *(char**)(esp+4);
		if(file == NULL){
			exit(-1);
		}
		check(file);
		returnValue = (uint32_t)remove(file);
	}else if(callNum == SYS_OPEN){
		check(esp+4);
		char* file = *(char**)(esp+4);
		if(file == NULL){
			exit(-1);
		}
		check(file);
		returnValue = (uint32_t)open(file);
	}else if(callNum == SYS_FILESIZE){
		check(esp+4);
		int fd = *(int*)(esp+4);
		checkFd(fd);
		returnValue = (uint32_t)getFileSize(fd);
	}else if(callNum == SYS_SEEK){
		check(esp+4); check(esp+8);
		int fd = *(int*)(esp+4);
		checkFd(fd);
		unsigned pos = *(unsigned*)(esp+8);
		seek(fd, pos);
	}else if(callNum == SYS_TELL){
		check(esp+4);
		int fd = *(int*)(esp+4);
		checkFd(fd);
		returnValue = (uint32_t)tell(fd);
	}else if(callNum == SYS_CLOSE){
		check(esp+4);
		int fd = *(int*)(esp+4);
		checkFd(fd);
		close(fd);
	}
	
	f->eax=returnValue;
}

