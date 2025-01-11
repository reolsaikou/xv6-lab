#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//
// This file contains copyin_new() and copyinstr_new(), the
// replacements for copyin and coyinstr in vm.c.
//

static struct stats {
  int ncopyin;
  int ncopyinstr;
} stats;

int
statscopyin(char *buf, int sz) {
  int n;
  n = snprintf(buf, sz, "copyin: %d\n", stats.ncopyin);
  n += snprintf(buf+n, sz, "copyinstr: %d\n", stats.ncopyinstr);
  return n;
}




// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  struct proc *p = myproc();
  uint64 va0,pa0,n;
  // pte_t *pte;
  int perm;

  
  if (srcva >= p->sz || srcva+len >= p->sz || srcva+len < srcva || srcva >= 0xC000000)
    return -1;

  memmove((void *) dst, (void *)srcva, len);

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    perm = walkperm(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;

    // perkvmmap(va0, pa0, PGSIZE, perm, p->kpagetable);
    // if(mappages(p->kpagetable, va0, PGSIZE, pa0, perm) < 0) return -1;
    if(vmcopyin_pg(pagetable, p->kpagetable, va0, pa0 ,perm) < 0) return -1;

    len -= n;
    srcva = va0 + PGSIZE;
  }

  stats.ncopyin++;   // XXX lock
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  struct proc *p = myproc();
  char *s = (char *) srcva;

  if (srcva >= p->sz || srcva+max < srcva || srcva >= 0xC000000)
    return -1;

  uint64 n, va0, pa0;
  int got_null = 0;
  int perm;
  uint64 va=srcva;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    perm = walkperm(pagetable, va0);

    if(pa0 == 0) return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;
    // perkvmmap(va0, pa0, PGSIZE, perm, p->kpagetable);
    if(vmcopyin_pg(pagetable, p->kpagetable, va0, pa0 ,perm) < 0) return -1;
    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        got_null = 1;
        break;
      }
      --n;
      --max;
      p++;
    }
    srcva = va0 + PGSIZE;
  }

  if(got_null == 0) return -1;
  stats.ncopyinstr++;   // XXX lock
  for(int i = 0; i < max && va + i < p->sz; i++){
    dst[i] = s[i];
    if(s[i] == '\0')
      return 0;
  }
  return -1;
}


