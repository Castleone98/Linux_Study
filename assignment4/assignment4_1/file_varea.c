#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <asm/syscall_wrapper.h>
#include <linux/mm.h>  // 추가된 헤더 파일
#include <linux/slab.h>

#define __NR_process_tracer 336 // 예시 시스템 호출 번호

void **syscall_table;
void *orig_process_tracer;

asmlinkage pid_t file_varea(pid_t trace_task)
{
    struct task_struct *task = &init_task;
    struct vm_area_struct *vma;
    int found = 0;
    char buf[256]; // 경로를 저장할 버퍼
    char *path;

    do
    {
        if (task->pid == trace_task)
        {
            found = 1;
            printk(KERN_INFO "####### Loaded files of a process '%s(%d)' in VM ########",  task->comm, task->pid);

            for (vma = task->mm->mmap; vma; vma = vma->vm_next)
            {

                if (vma->vm_file)
                {
                    printk(KERN_INFO "mem[%lx~%lx] ", vma->vm_start, vma->vm_end);
                    printk(KERN_CONT "code[%lx~%lx] ", task->mm->start_code, task->mm->end_code);
                    printk(KERN_CONT "data[%lx~%lx] ", task->mm->start_data, task->mm->end_data);
                    printk(KERN_CONT "heap[%lx~%lx] ", task->mm->start_brk, task->mm->brk);
                    path = d_path(&vma->vm_file->f_path, buf, sizeof(buf));
                    if (!IS_ERR(path))
                    {
                        printk(KERN_CONT "%s\n", path);
                    }
                }
            }
            break;
        }
        task = next_task(task);
    } while (task != &init_task);

    if (!found)
    {
        printk(KERN_INFO "##### PROCESS NOT FOUND #####\n");
        return -1;
    }

    printk(KERN_INFO "#########################################################\n");
    
    return trace_task;
}

__SYSCALL_DEFINEx(1, ftrace, pid_t, pid)
{
    return file_varea(pid);
}

void make_rw(void *addr)
{
    unsigned int level;
    pte_t *pte = lookup_address((u64)addr, &level);
    if (pte->pte & ~_PAGE_RW)
    {
        pte->pte |= _PAGE_RW;
    }
}

void make_ro(void *addr)
{
    unsigned int level;
    pte_t *pte = lookup_address((u64)addr, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
}

static int __init hooking_init(void)
{
    syscall_table = (void **)kallsyms_lookup_name("sys_call_table");
    make_rw(syscall_table);
    orig_process_tracer = syscall_table[__NR_process_tracer];
    syscall_table[__NR_process_tracer] = __x64_sysftrace;
    return 0;
}

static void __exit hooking_exit(void)
{
    syscall_table[__NR_process_tracer] = orig_process_tracer;
    make_ro(syscall_table);
}

module_init(hooking_init);
module_exit(hooking_exit);

MODULE_LICENSE("GPL");
