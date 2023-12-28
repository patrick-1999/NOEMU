#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdio.h>

#define Machine_external_interrupt 0xb

// uintptr_t mepc, mstatus, mcause, gpr[NR_REGS] ;
//   void *pdir;
static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {

  if (user_handler) {
    Event ev = {0};
    // printf("c->mcause:%x\n",c->mcause);
    switch (c->mcause) {
      case 8:
      case Machine_external_interrupt:
      // 通过查阅<man 2 syscall>可以知道
      // riscv: 
      // system call:a7 
      // ret val:a1 
      // ret val2:a2
        if (c->gpr[17] == -1) {
          // 如果没有系统调用好是自陷
          ev.event = EVENT_YIELD;
          printf("EVENT_YIELD\n");
        } else {
          // 如果有系统调用号就是系统调用
          ev.event = EVENT_SYSCALL;
        }
        c->mepc += 4;     // skip the instruction that caused the trap
        // 根据造成这个ecall的成因判断是否需要给PC+4
        // 有的时候例如缺页异常需要回到原来的指令执行就不用+4,如果要跳过触发异常的指令就可以+4
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}


Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context* context = kstack.end - sizeof(Context);
  memset(context, 0, sizeof(Context));
  // 关键
  context->mepc = (uintptr_t)entry;

  context->mstatus = 0x1800; 
  // enable int after context switch, but not in trap
  // so set mpie not mie, or the next intr may trigger intr though in trap
  context->mcause = 0;
  context->GPRx = (uintptr_t)arg;
  return context;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
