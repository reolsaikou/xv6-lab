// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define STEAL_NUM 32

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct spinlock steallock;

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

char kmem_name[NCPU][16];
int kmem_cnt[NCPU];

void
kinit()
{
  for(int i=0;i < NCPU;i++){
    snprintf(kmem_name[i], 15, "kmem%d", i);
    initlock(&kmem[i].lock, kmem_name[i]);
  }
  // initlock(&kmem.lock, "kmem");
  initlock(&steallock, "steal");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  for(int i=0;i < NCPU; i++){
    kmem_cnt[i] = 0;
  }
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int cpu = cpuid();
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  kmem_cnt[cpu]++;
  release(&kmem[cpu].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpu = cpuid();
  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  
  if(r){
    kmem[cpu].freelist = r->next;
    kmem_cnt[cpu]--;
    release(&kmem[cpu].lock);
  } else{
    release(&kmem[cpu].lock);
    if(ksteal(cpu)){
      r = kmem[cpu].freelist;
      kmem[cpu].freelist = r->next;
      kmem_cnt[cpu]--;
      release(&kmem[cpu].lock);
    }
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// only one cpu steal at one time
int
ksteal(int cpu){
  struct run *r;
  acquire(&steallock);
  int cnt = STEAL_NUM;
  for(int i=0;i<NCPU; i++){
    if(i == cpu) continue;
    acquire(&kmem[i].lock);
    if(kmem_cnt[i] > 0){
      acquire(&kmem[cpu].lock);
      r = kmem[i].freelist;
      while((cnt--) && (r)){
        kmem[i].freelist = r->next;
        r->next = kmem[cpu].freelist;
        kmem[cpu].freelist = r;
        kmem_cnt[cpu]++;
        kmem_cnt[i]--;
        r = kmem[i].freelist;
      }
      release(&kmem[i].lock);
      release(&steallock);
      return 1;
    }
    release(&kmem[i].lock);
  }
  release(&steallock);
  return 0;
}