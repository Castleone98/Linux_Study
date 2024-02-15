#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <asm/syscall_wrapper.h>

#define __NR_process_tracer 336 // 예시 시스템 호출 번호

void **syscall_table;
void *orig_process_tracer;

asmlinkage pid_t process_tracer(pid_t trace_task)
{
    struct task_struct *task = &init_task;
    struct list_head *p;
    int found = 0;
    int sibling_count = 0;
    int child_count = 0;

    printk(KERN_INFO "##### START OF INFORMATION #####\n");

    do
    {

        if (task->pid == trace_task)
        {
            found = 1;
            printk(KERN_INFO "##### TASK INFORMATION OF [%d] %s #####\n", task->pid, task->comm);

            // Process state in human-readable format
            switch (task->state)
            {
            case TASK_RUNNING:
                printk(KERN_INFO "- task state : Running\n");
                break;
            case TASK_INTERRUPTIBLE:
                printk(KERN_INFO "- task state : Wait\n");
                break;
            case TASK_UNINTERRUPTIBLE:
                printk(KERN_INFO "- task state : Wait (ignoring all signals)\n");
                break;
            case TASK_STOPPED:
                printk(KERN_INFO "- task state : Stopped\n");
                break;
            case EXIT_ZOMBIE:
                printk(KERN_INFO "- task state : Zombie\n");
                break;
            default:
                printk(KERN_INFO "- task state : Unknown (%ld)\n", task->state);
            }

            printk(KERN_INFO "- Process Group Leader : [%d] %s\n", task->group_leader->pid, task->group_leader->comm);

            // Number of context switches
            printk(KERN_INFO "- Number of context switches : %lu\n", task->nvcsw + task->nivcsw);

            // TODO: Number of calling fork()
            printk(KERN_INFO "- Number of calling fork() : %d\n", task->fork_count);

            // Parent process
            if (task->parent)
            {
                printk(KERN_INFO "- it's parent process : [%d] %s\n", task->parent->pid, task->parent->comm);
            }

            // Sibling processes
            printk(KERN_INFO "- it's sibling process(es) :\n");
            list_for_each(p, &task->sibling)
            {
                struct task_struct *sibling;
                sibling = list_entry(p, struct task_struct, sibling);
                if (sibling->pid != task->parent->pid)
                { // 현재 프로세스를 제외
                    printk(KERN_INFO "  [%d] %s\n", sibling->pid, sibling->comm);
                    sibling_count++;
                }
            }

            printk(KERN_INFO "> This process has %d sibling process(es)\n", sibling_count);

            // Child processes
            child_count = 0;
            printk(KERN_INFO "- it's child process(es) :\n");
            list_for_each(p, &task->children)
            {
                struct task_struct *child;
                child = list_entry(p, struct task_struct, sibling);
                printk(KERN_INFO "  [%d] %s\n", child->pid, child->comm);
                child_count++;
            }
            printk(KERN_INFO "> This process has %d child process(es)\n", child_count);

            break;
        }
        task = next_task(task);
    } while (task != &init_task);

    if (!found)
    {
        printk(KERN_INFO "##### PROCESS NOT FOUND #####\n");
        return -1;
    }

    printk(KERN_INFO "##### END OF INFORMATION #####\n");

    return trace_task;
}
__SYSCALL_DEFINEx(1, ftrace, pid_t, pid)
{
    return process_tracer(pid);
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
