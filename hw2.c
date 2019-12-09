#include <linux/module.h> //module_init, module_exit
#include <linux/seq_file.h> // seq_printf, single_open
#include <linux/proc_fs.h> // proc_remove
#include <linux/sched/signal.h> // for_each_process
#include <linux/moduleparam.h> // module_param 
#include <linux/time.h> // to implement timer
#include <linux/jiffies.h> // jiffies
#include <linux/interrupt.h> // tasklet
#include <asm/pgtable.h> // mm

static int period = 5; // initialization
module_param(period, int, 0);
static struct timer_list timer;

void tasklet_handler(unsigned long);
DECLARE_TASKLET(ts, tasklet_handler, 0); //declare tasklet

static struct proc_dir_entry *hw2_proc_dir = NULL;
// static struct proc_dir_entry *hw2_proc_f
static struct VMAI {
    char name[TASK_COMM_LEN];
    pid_t pid;
    int lut; // jiffies_to_msecs(jiffies)

    unsigned long ca_s, ca_e, da_s, da_e, ba_s, ba_e, ha_s, ha_e, sla_s, sla_e, sa_s, sa_e;
    unsigned long ca_p, da_p, ba_p, ha_p, sla_p, sa_p;

    unsigned long pgd_ba, pgd_a, pgd_v, pgd_pfna;
    char* pgd_ps, pgd_ab, pgd_cdb, pgd_pwt, pgd_usb, pgd_rwb, pgd_ppb;

    unsigned long pud_a, pud_v, pud_pfna;

    unsigned long pmd_a, pmd_v, pmd_pfna;

    unsigned long pte_a, pte_v, pte_pba;
    char* pte_db, pte_ab, pte_cdb, pte_pwt, pte_usb, pte_rwb, pte_ppb;

    unsigned long phy_a;
} vmai[32769];

static int hw2_single_show(struct seq_file *s, void *unused){
    const unsigned char *c = s->file->f_path.dentry->d_name.name;
    char **ep;
    int i = simple_strtol(c, ep, 10);
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "Virtual Memory Address Information\n");
    seq_printf(s, "Process (%15s:%lu)\n", vmai[i].name, vmai[i].pid);
    seq_printf(s, "Last update time %llu ms\n", vmai[i].lut);
    seq_printf(s, "************************************************************\n");

    // print info about each area
    seq_printf(s, "0x%08lx - 0x%08lx : Code Area, %lu page(s)\n",
    vmai[i].ca_s, vmai[i].ca_e, vmai[i].ca_p);
    seq_printf(s, "0x%08lx - 0x%08lx : Data Area, %lu page(s)\n",
    vmai[i].da_s, vmai[i].da_e, vmai[i].da_p);
    seq_printf(s, "0x%08lx - 0x%08lx : BSS Area, %lu page(s)\n",
    vmai[i].ba_s, vmai[i].ba_e, vmai[i].ba_p);
    seq_printf(s, "0x%08lx - 0x%08lx : Heap Area, %lu page(s)\n",
    vmai[i].ha_s, vmai[i].ha_e, vmai[i].ha_p);
    seq_printf(s, "0x%08lx - 0x%08lx : Shared Libraries Area, %lu page(s)\n",
    vmai[i].sla_s, vmai[i].sla_e, vmai[i].sla_p);
    seq_printf(s, "0x%08lx - 0x%08lx : Stack Area, %lu page(s)\n",
    vmai[i].sa_s, vmai[i].sa_e, vmai[i].sa_p);

    // 1 level paging (PGD Info)
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "1 Level Paging: Page Global Directory Entry Information \n");
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "PGD     Base Address            : 0x%08lx\n", vmai[i].pgd_ba);
    seq_printf(s, "code    PGD Address             : 0x%08lx\n", vmai[i].pgd_a);
    seq_printf(s, "        PGD Value               : 0x%08lx\n", vmai[i].pgd_v);
    seq_printf(s, "        +PFN Address            : 0x%08lx\n", vmai[i].pgd_pfna);
    seq_printf(s, "        +Page Size              : %s\n", vmai[i].pgd_ps);
    seq_printf(s, "        +Accessed Bit           : %s\n", vmai[i].pgd_ab);
    seq_printf(s, "        +Cache Disable Bit      : %s\n", vmai[i].pgd_cdb);
    seq_printf(s, "        +Page Write-Through     : %s\n", vmai[i].pgd_pwt);
    seq_printf(s, "        +User/Supervisor Bit    : %s\n", vmai[i].pgd_usb);
    seq_printf(s, "        +Read/Write Bit         : %s\n", vmai[i].pgd_rwb);
    seq_printf(s, "        +Page Present Bit       : %s\n", vmai[i].pgd_ppb);

    // 2 level paging (PUD Info)
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "2 Level Paging: Page Upper Directory Entry Information \n");
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "code    PUD Address             : 0x%08lx\n", vmai[i].pud_a);
    seq_printf(s, "        PUD Value               : 0x%08lx\n", vmai[i].pud_v);
    seq_printf(s, "        +PFN Address            : 0x%08lx\n", vmai[i].pud_pfna);

    // 3 level paging (PMD Info)
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "3 Level Paging: Page Middle Directory Entry Information \n");
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "code    PMD Address             : 0x%08lx\n", vmai[i].pmd_a);
    seq_printf(s, "        PMD Value               : 0x%08lx\n", vmai[i].pmd_v);
    seq_printf(s, "        +PFN Address            : 0x%08lx\n", vmai[i].pmd_pfna);

    // 4 level paging (PTE Info)
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "4 Level Paging: Page Table Entry Information \n");
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "code    PTE Address             : 0x%08lx\n", vmai[i].pte_a);
    seq_printf(s, "        PTE Value               : 0x%08lx\n", vmai[i].pte_v);
    seq_printf(s, "        +Page Base Address      : 0x%08lx\n", vmai[i].pte_pba);
    seq_printf(s, "        +Dirty Bit              : %s\n", vmai[i].pte_db);
    seq_printf(s, "        +Accessed Bit           : %s\n", vmai[i].pte_ab);
    seq_printf(s, "        +Cache Disable Bit      : %s\n", vmai[i].pte_cdb);
    seq_printf(s, "        +Page Write-Through     : %s\n", vmai[i].pte_pwt);
    seq_printf(s, "        +User/Supervisor        : %s\n", vmai[i].pte_usb);
    seq_printf(s, "        +Read/Write Bit         : %s\n", vmai[i].pte_rwb);
    seq_printf(s, "        +Page Present Bit       : %s\n", vmai[i].pte_ppb);
    seq_printf(s, "************************************************************\n");
    seq_printf(s, "Start of Physical Address       : 0x%08lx\n", vmai[i].phy_a);
    seq_printf(s, "************************************************************\n");
}

static int hw2_open(struct inode *inode, struct file *file){
    return single_open(file, hw2_single_show, NULL); // print the whole output at once
}

static struct file_operations hw2_proc_ops = {
    .open = hw2_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

void tasklet_handler(unsigned long data){
    // remove and make directory /proc/hw2
    proc_remove(hw2_proc_dir);
    hw2_proc_dir = proc_mkdir("hw2", NULL);
    struct task_struct* p;
    struct mm_struct* m;
    int a=0;
    int i;
    int last_update_time = jiffies_to_msecs(jiffies);
    for_each_process(p) {
        m = p->active_mm;

        // for kernel threads, mm is always NULL
        if (!m) continue;

        i = p->pid;

        strncpy(vmai[i].name, p->comm, TASK_COMM_LEN);

        // pid, last update time
        vmai[i].pid = i;
        vmai[i].lut = last_update_time;

        // Each area info
        vmai[i].ca_s = m->start_code;
        vmai[i].ca_e = m->end_code;
        vmai[i].ca_p = (vmai[i].ca_e-vmai[i].ca_s) / 4096;

        vmai[i].da_s = m->start_data;
        vmai[i].da_e = m->end_data;
        vmai[i].da_p = (vmai[i].da_e-vmai[i].da_s) / 4096;

        // vmai[i].ba_s = m->;
        // vmai[i].ba_e = m->;
        vmai[i].ba_p = (vmai[i].ba_e-vmai[i].ba_s) / 4096;

        vmai[i].ha_s = m->start_brk;
        vmai[i].ha_e = m->brk;
        vmai[i].ha_p = (vmai[i].ha_e-vmai[i].ha_s) / 4096;

        // vmai[i].sla_s = m->;
        // vmai[i].sla_e = m->;
        vmai[i].sla_p = (vmai[i].sla_e-vmai[i].sla_s) / 4096;

        vmai[i].sa_s = m->start_stack;
        // vmai[i].sa_e = m->start_stack; vm end
        vmai[i].sa_p = (vmai[i].sa_e-vmai[i].sa_s) / 4096;

        // 1 level paging (PGD Info)
        unsigned long msb_clear;
        vmai[i].pgd_ba = m->pgd;
        vmai[i].pgd_a = pgd_offset(m, vmai[i].ca_s);
        vmai[i].pgd_v = (unsigned) pgd_val(*pgd_offset(m, vmai[i].ca_s));
        msb_clear = (vmai[i].pgd_v << 1) >> 1;
        vmai[i].pgd_pfna = msb_clear >> PAGE_SHIFT;

        vmai[i].pgd_ps = ((vmai[i].pgd_v >> 7)%2 == 0) ? "4KB" : "4MB";
        vmai[i].pgd_ab = ((vmai[i].pgd_v >> 5)%2 == 0) ? "0" : "1";
        vmai[i].pgd_cdb = ((vmai[i].pgd_v >> 4)%2 == 0) ? "false" : "true";
        vmai[i].pgd_pwt = ((vmai[i].pgd_v >> 3)%2 == 0) ? "write-back" : "write-through";
        vmai[i].pgd_usb = ((vmai[i].pgd_v >> 2)%2 == 0) ? "supervisor" : "user";
        vmai[i].pgd_rwb = ((vmai[i].pgd_v >> 1)%2 == 0) ? "read-only" : "read-write";
        vmai[i].pgd_ppb = (vmai[i].pgd_v%2 == 0) ? "0" : "1";

        if ((vmai[i].pgd_v >> 7)%2 == 1){ // if page size is 4MB
            vmai[i].ca_p /= 1024;
            vmai[i].da_p /= 1024;
            vmai[i].ba_p /= 1024;
            vmai[i].ha_p /= 1024;
            vmai[i].sla_p /= 1024;
            vmai[i].sa_p /= 1024;
        }

        // 2 level paging (PUD Info)
        vmai[i].pud_a = pud_offset(p4d_offset(vmai[i].pgd_a, vmai[i].ca_s), vmai[i].ca_s);
        vmai[i].pud_v = (unsigned) pud_val(*pud_offset(p4d_offset(vmai[i].pgd_a, vmai[i].ca_s), vmai[i].ca_s));
        msb_clear = (vmai[i].pud_v << 1) >> 1;
        vmai[i].pud_pfna = msb_clear >> PAGE_SHIFT;

        // 3 level paging (PMD Info)
        vmai[i].pmd_a = pmd_offset(vmai[i].pud_a, vmai[i].ca_s);
        vmai[i].pmd_v = (unsigned) pmd_val(*pmd_offset(vmai[i].pud_a, vmai[i].ca_s));
        msb_clear = (vmai[i].pmd_v << 1) >> 1;
        vmai[i].pmd_pfna = msb_clear >> PAGE_SHIFT;

        // 4 level paging (PTE Info)
        vmai[i].pte_a = pte_offset_kernel(vmai[i].pmd_a, vmai[i].ca_s);
        vmai[i].pte_v = (unsigned) pte_val(*pte_offset_kernel(vmai[i].pmd_a, vmai[i].ca_s));
        msb_clear = (vmai[i].pte_v << 1) >> 1;
        vmai[i].pte_pba = msb_clear >> PAGE_SHIFT;

        vmai[i].pte_db = ((vmai[i].pgd_v >> 6)%2 == 0) ? "0" : "1";
        vmai[i].pte_ab = ((vmai[i].pgd_v >> 5)%2 == 0) ? "0" : "1";
        vmai[i].pte_cdb = ((vmai[i].pgd_v >> 4)%2 == 0) ? "false" : "true";
        vmai[i].pte_pwt = ((vmai[i].pgd_v >> 3)%2 == 0) ? "write-back" : "write-through";
        vmai[i].pte_usb = ((vmai[i].pgd_v >> 2)%2 == 0) ? "supervisor" : "user";
        vmai[i].pte_rwb = ((vmai[i].pgd_v >> 1)%2 == 0) ? "read-only" : "read-write";
        vmai[i].pte_ppb = (vmai[i].pgd_v%2 == 0) ? "0" : "1";

        // Physical address
        vmai[i].phy_a = (vmai[i].pte_pba << PAGE_SHIFT) + vmai[i].ca_s;

        char name[6];
        snprintf(name, sizeof(name), "%d", i);
        printk("before: %d", i);
        proc_create(name, 644, hw2_proc_dir, &hw2_proc_ops);
        printk("after: %d", i);

        a++;
        // printk("Current process pid: %d\n", p->pid);
    }
    // printk("%d %s\n", vmai[1].pid, vmai[1].name);
    printk("Tasklet running! Number of processes: %d\n", a);
}

void timer_handler(struct timer_list *t){
    tasklet_hi_schedule(&ts);

    // reschedule timer
    mod_timer(&timer, jiffies + msecs_to_jiffies(period*1000));
}

int hw2_proc_init(void){
    // initialize timer
    timer_setup(&timer, timer_handler, 0); 

    hw2_proc_dir = proc_mkdir("hw2", NULL);

    // schedule timer. when expires, run handler function(timer_handler)
    mod_timer(&timer, jiffies + msecs_to_jiffies(period*1000));
    
    return 0;
}

void hw2_proc_exit(void){
    tasklet_kill(&ts);
    del_timer(&timer);
    proc_remove(hw2_proc_dir);
}

module_init(hw2_proc_init);
module_exit(hw2_proc_exit);