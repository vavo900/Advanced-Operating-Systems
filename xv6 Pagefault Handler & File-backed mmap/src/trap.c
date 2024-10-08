#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "mman.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{  
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void
pagefault_handler(struct trapframe *tf)
{
  
  mmap_reg * node_test = 0;
  node_test = myproc()->reg_frst;

  void* fault_addr = (void*)PGROUNDDOWN(rcr2());
  
  char *m = 0;
  int cnt = 0;
  

  while((int)node_test->addr % PGSIZE == 0)
  {
    cnt++;

    if(fault_addr >= node_test->addr && fault_addr < (node_test->addr + node_test->len + PGSIZE))
    {
      if(node_test->prot & PROT_WRITE) 
      {

        m = kalloc();

        if((int)m == 0)
        {
          return;
        }

        memset(m, 0 , PGSIZE);

        mappages(myproc()->pgdir, (char*)fault_addr, PGSIZE, V2P(m), PTE_W|PTE_U); 
        lcr3(V2P(myproc()->pgdir));
        
      } 
      else {
        if(tf->err & 0x2)
        {       
          break;
        }

        m = kalloc();
        if((int)m == 0)
        {
          return;
        }

        memset(m, 0 , PGSIZE);

        mappages(myproc()->pgdir, (char*)fault_addr, PGSIZE, V2P(m), PTE_U);
        lcr3(V2P(myproc()->pgdir));
        
      }
      if(node_test->reg_type == MAP_FILE)
      {
        
        fileseek(myproc()->ofile[node_test->fd], node_test->offset);

        fileread(myproc()->ofile[node_test->fd], m, PGSIZE);
        
        pte_t *pt = walkpgdir(myproc()->pgdir, fault_addr, 0);

        if(pt)
        {
          *pt &= ~PTE_D;
        }
      }
      return;
    }

    node_test = node_test->reg_nxt;
  }
  
  if(myproc() == 0 || (tf->cs&3) == 0){
      panic("trap");
    }
    
    cprintf("============in pagefault_handler============\n");
    cprintf("pid %d %s: trap %d err %d on cpu %d "
          "eip 0x%x addr 0x%x\n",
          myproc()->pid, myproc()->name, tf->trapno,
          tf->err, cpuid(), tf->eip, fault_addr);

    myproc()->killed = 1;
  }

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    pagefault_handler(tf);
    return;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
