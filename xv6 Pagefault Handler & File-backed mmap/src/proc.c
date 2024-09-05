#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "mman.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "stat.h"


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  p->reg_cnt = 0;
  p->free_cnt = 0;
  p->reg_free = 0;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->free_cnt = curproc->free_cnt;
  np->reg_cnt = curproc->reg_cnt;

  if(np->reg_cnt > 0)
  {
    mmap_reg *reg_prv =  curproc->reg_frst;
    mmap_reg *reg_prv_nw;
    mmap_reg *frst_reg = kmalloc(sizeof(mmap_reg));
    *frst_reg = *reg_prv;

    if(frst_reg->reg_type == MAP_FILE){
      int fd;
      if((fd=fdalloc(curproc->ofile[frst_reg->fd])) < 0)
        return -1;

      filedup(curproc->ofile[fd]);
      frst_reg->fd = fd;
    }
    np->reg_frst = frst_reg;
    reg_prv = reg_prv->reg_nxt;

    reg_prv_nw = np->reg_frst;

    while ((int)reg_prv->addr % PGSIZE == 0)
    {
      mmap_reg *reg_nw = kmalloc(sizeof(mmap_reg));

      *reg_nw = *reg_prv;
      if(reg_nw->reg_type == MAP_FILE)
      {
        int fd;
        if((fd=fdalloc(curproc->ofile[reg_nw->fd])) < 0)
          return -1;

        filedup(curproc->ofile[fd]);
        frst_reg->fd = fd;
      }
      *reg_prv_nw->reg_nxt = *reg_nw;
      reg_prv_nw = reg_prv_nw->reg_nxt;

      reg_prv = reg_prv -> reg_nxt;
    }
  }

  if(np->free_cnt >0){

    mmap_reg * reg_prv = curproc->reg_free;
    mmap_reg *reg_prv_nw;

    mmap_reg *reg_frst_free = kmalloc(sizeof(mmap_reg));
    *reg_frst_free = *reg_prv;
    np->reg_free = reg_frst_free;

    reg_prv = reg_prv->reg_nxt;
    reg_prv_nw = np->reg_free;

    while ((int)reg_prv->addr % PGSIZE == 0)
    {
      mmap_reg *reg_nw = kmalloc(sizeof(mmap_reg));
      *reg_nw = *reg_prv;
      *reg_prv_nw->reg_nxt = *reg_nw;

      reg_prv_nw = reg_prv_nw->reg_nxt;
      reg_prv = reg_prv -> reg_nxt;
    }
  }
  

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

void* mmap(void* addr, int length, int prot, int flags, int fd, int offset){
  
  struct proc *curproc = myproc();

  void *addr_cls =0;
  void *addr_ret =0;

  mmap_reg *spc_free=0;
  mmap_reg *reg_cls=0;

  mmap_reg *prv_free =0;
  mmap_reg *prv_cls =0;

  int dist;


  if(length < 1)
  {
    return (void*)-1;
  }

  if(flags == MAP_ANONYMOUS)
  { 
    if(fd != -1){
      return (void*)-1;
    }

  }
  if(flags == MAP_FILE){

    if(fd < 0 || fd >= NOFILE || ((int)curproc->ofile[fd] == 0) || curproc->ofile[fd]->type != FD_INODE || curproc->ofile[fd]->ip->type != T_FILE)
    {
      return (void*)-1;
    }
    
    if((fd=fdalloc(curproc->ofile[fd])) < 0)
      return (void*)-1;


    filedup(curproc->ofile[fd]);
    
  }

  length = PGROUNDUP(length);
  dist = curproc->sz - (uint)addr;

  int addr_rnd = PGROUNDUP((int)addr);

  if(curproc->free_cnt > 0)
  {
    spc_free = curproc->reg_free;
    
    while(1){
      if((int)spc_free->addr % PGSIZE != 0)
      {
        break;
      }
      if(spc_free->len >= length)
      {
        if (dist > spc_free->addr - addr)
        {
          addr_cls = spc_free->addr;
          reg_cls = spc_free;
          prv_cls = prv_free;
          dist = spc_free->addr - addr;
        }
      }
    
      prv_free = spc_free; 
      spc_free =spc_free->reg_nxt;
    }
    
    if(reg_cls->len == length)
    { 
      if(prv_cls == 0){ 
        curproc->reg_free= curproc->reg_free->reg_nxt;
      } else{ 
        prv_cls->reg_nxt = reg_cls->reg_nxt;
      }
      kmfree(reg_cls);  

      curproc->free_cnt -= 1;
    } else { 
      
      while(addr_rnd > (int)reg_cls->addr )
      {
        if((uint)addr_rnd + length < (uint)reg_cls->addr+reg_cls->len){
          addr_cls = (void*)addr_rnd;

          mmap_reg *reg_nxt_nw = kmalloc(sizeof(mmap_reg));
          reg_nxt_nw->reg_nxt = reg_cls->reg_nxt;

          reg_cls->reg_nxt = reg_nxt_nw;
          
          reg_nxt_nw->addr = (void*)addr_rnd+length; 
          reg_nxt_nw->len = reg_cls->len - ((addr_rnd - (int)reg_cls->addr) +length);

          reg_cls->len = addr_rnd - (int)reg_cls->addr;
          break;

        } else
        {
          addr_rnd -= PGSIZE;
        } 
      }
      if(addr_rnd < (int)reg_cls->addr){
        addr_cls = reg_cls->addr;
        reg_cls->addr += length;

        reg_cls->len -= length;
      }
      if(prv_cls == 0)
      {
        curproc->reg_free = reg_cls;
      }
      if (reg_cls->len == 0)
      {
        prv_cls->reg_nxt = reg_cls->reg_nxt;
        kmfree(reg_cls);
      }
      if (reg_cls->reg_nxt->len == 0){

        reg_cls->reg_nxt = reg_cls->reg_nxt->reg_nxt;
        kmfree(reg_cls->reg_nxt);
      }
    }
    addr_ret = (void*)addr_cls;

    } else{
      addr_ret = (void*)curproc->sz;

      curproc->sz += length;
    }

  
  mmap_reg *reg_curr = kmalloc(sizeof(mmap_reg));
  
  reg_curr->addr = addr_ret;
  reg_curr->len = length;
  reg_curr->reg_type = flags;

  reg_curr->offset = offset;
  reg_curr->fd = fd;
  reg_curr->prot = prot;
  
  if(curproc->reg_cnt == 0)
  { 
    curproc->reg_frst = reg_curr;

    reg_curr->reg_nxt = 0;
  } else if (curproc->reg_frst->addr > addr_ret) 
  { 
    reg_curr->reg_nxt = curproc->reg_frst;
    curproc->reg_frst = reg_curr;
  } 
  else 
  {  
    mmap_reg *reg_prv = curproc->reg_frst, *tnode;

    for(tnode = curproc->reg_frst->reg_nxt; ; tnode = tnode->reg_nxt)
    {
      if(tnode->addr > addr_ret)
      {
        reg_curr->reg_nxt = tnode;
        reg_prv->reg_nxt = reg_curr;
        break;
      } else if (tnode->reg_nxt == 0){
        tnode->reg_nxt = reg_curr;

        reg_curr->reg_nxt = 0;
        break;
      }
      reg_prv = tnode;

    }
  }
  curproc->reg_cnt += 1;

  return addr_ret;
}

int munmap(void* addr, uint length)
{
  struct proc *curproc = myproc();
  mmap_reg *reg_prv = curproc->reg_frst; 

  length = PGROUNDUP(length);
  mmap_reg *reg_mtch = 0;
  

  if(curproc->reg_cnt == 0)
  { 
    return -1;
  }

  if ((int)addr % 4096 != 0){
    return -1;
  }

  
  reg_prv = curproc->reg_frst;
  int cnt = curproc->reg_cnt;
  
  while(cnt > 0){
    if(curproc->reg_frst->addr == addr && curproc->reg_frst->len == length){ 
      reg_mtch = curproc->reg_frst;

      if(cnt > 1)
      {
        curproc->reg_frst = curproc->reg_frst->reg_nxt;
      } else {
        curproc->reg_frst = 0;
      }
    break;
    }
    if (reg_prv->reg_nxt->addr == addr && reg_prv->reg_nxt->len == length){ 
      reg_mtch = reg_prv->reg_nxt; 

      reg_prv->reg_nxt = reg_mtch->reg_nxt; 
      break;
    }
    if((int)reg_prv->reg_nxt->addr % PGSIZE != 0){ 

      return -1;
    }
    reg_prv = reg_prv->reg_nxt;

    cnt --;
  }

  
  pte_t *pt;

  if (reg_mtch != 0)
  {
    if((pt = walkpgdir(curproc->pgdir, reg_mtch->addr, 0)) == 0)
    { 
      memset(addr, 0, length); 
    }
    if(reg_mtch->reg_type == MAP_FILE){
      fileclose(curproc->ofile[reg_mtch->fd]);
      curproc->ofile[reg_mtch->fd] = 0;
    }
    deallocuvm(curproc->pgdir, (uint)reg_mtch->addr +length, (uint)reg_mtch->addr);
    lcr3(V2P(curproc->pgdir));

    curproc->reg_cnt--;
    kmfree(reg_mtch);
  }

  mmap_reg *nxt_nd;
  mmap_reg *ret = kmalloc(sizeof(mmap_reg)); 

  mmap_reg *prv_nd;

  ret->addr = addr;
  ret->len = length;

  if(curproc->free_cnt == 0)
  { 
    curproc->reg_free = ret;
    curproc->reg_free->reg_nxt = 0;
  } else{
    prv_nd = curproc->reg_free;

    while(1){
      if(addr > prv_nd->addr && (addr < prv_nd->reg_nxt->addr || prv_nd->reg_nxt == 0))
      { 
        if(prv_nd->addr + prv_nd->len == ret->addr){ 
          prv_nd->len += ret->len;

          if(ret->addr + ret->len == prv_nd->reg_nxt->addr){
            prv_nd->len += prv_nd->reg_nxt->len;
            prv_nd->reg_nxt = prv_nd->reg_nxt->reg_nxt;

            break;
          }
          break;
        } 
        else if (ret->addr + ret->len == prv_nd->reg_nxt->addr)
        {
          ret->len += prv_nd->reg_nxt-> len;
          ret->reg_nxt = prv_nd->reg_nxt->reg_nxt;

          prv_nd->reg_nxt = ret;
          break;
        } else{
          nxt_nd = prv_nd->reg_nxt;
          prv_nd->reg_nxt = ret;

          ret->reg_nxt = nxt_nd;
          break;
        }
      }
      if(prv_nd->reg_nxt == 0)
      {
        prv_nd->reg_nxt = ret;
        ret->reg_nxt = 0;

        break;
      }
      prv_nd = prv_nd->reg_nxt;

    }
  }
  curproc->free_cnt += 1;

  return 0;
}

int msync(void * start_addr, int length){
  
  struct proc *curproc = myproc();

  mmap_reg *prv_nd = curproc->reg_frst; 
  mmap_reg *reg_mtch = 0;

  int cnt = curproc->reg_cnt;
  int lng_rnd = PGROUNDUP(length);
  
  if(curproc->reg_cnt == 0){
    return -1;
  }

  while(cnt > 0){
    
    if(curproc->reg_frst->addr == start_addr && curproc->reg_frst->len == lng_rnd){
      reg_mtch = curproc->reg_frst;
    break;
    }
    if (prv_nd->reg_nxt->addr == start_addr && prv_nd->reg_nxt->len == lng_rnd){ 
      reg_mtch = prv_nd->reg_nxt; 
      break;
    }
    if((int)prv_nd->reg_nxt->addr % PGSIZE != 0){ 
      return -1;
    }
    prv_nd = prv_nd->reg_nxt;
    cnt --;
  }

  if(fileseek(curproc->ofile[reg_mtch->fd], reg_mtch->offset) != 0){  
    return -1;
  }

  
  pte_t *pt;
  void *chk_pg = reg_mtch->addr;

  int cnt_val = 0;

  while (chk_pg < reg_mtch->addr + reg_mtch->len)
  {
    cnt_val ++;
    pt = walkpgdir(curproc->pgdir, chk_pg, 0);

    if(pt != 0 && *pt & PTE_D)
    {
      if(length >= PGSIZE)
      {
        filewrite(curproc->ofile[reg_mtch->fd], chk_pg, PGSIZE);
      } else {
        filewrite(curproc->ofile[reg_mtch->fd], chk_pg, length);
      }
    } else {
      
      if(fileseek(curproc->ofile[reg_mtch->fd], reg_mtch->offset + cnt_val * PGSIZE) != 0){
        return -1;
      } 
    }
    length -= PGSIZE;

    chk_pg = (void *)((int)chk_pg + PGSIZE);  
  }
  return 0;
}
