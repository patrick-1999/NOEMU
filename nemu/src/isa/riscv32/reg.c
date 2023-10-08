/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "local-include/reg.h"
#include<stdio.h>

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};


void isa_reg_display() {
  for (int i = 0; i < 32; i++) {
    printf("%3s = 0x%08x\t", regs[i], cpu.gpr[i]);
    if ((i+1) % 4 == 0) {
      printf("\n");
    }
  }
  printf("pc = 0x%x\t\n", cpu.pc);
  printf("mcause:%x\n",cpu.csr.mcause);
  printf("mstatus:%x\n",cpu.csr.mstatus);
  printf("mepc:%x\n",cpu.csr.mepc);
  
  
}

word_t isa_reg_str2val(const char *s, bool *success) {

    if (!strcmp(s, "0")) {\
    return cpu.gpr[0];
  }
  if (!strcmp(s, "pc")) {
    return cpu.pc;
  }
  if (!strcmp(s, "mcause")) {
    return cpu.csr.mcause;
  }
  if (!strcmp(s, "mstatus")) {
    return cpu.csr.mstatus;
  }
  if (!strcmp(s, "mepc")) {
    return cpu.csr.mepc;
  }
  for (int i = 1; i < 32; i++) {
    if (!strcmp(s, regs[i])) {
      // printf("return others:%x",cpu.gpr[i]);
      return cpu.gpr[i];
    }
  }
  *success = false;
  printf("<%s>reg unknow!\n",s);
  // printf("cmp%d\n",!strcmp(s, "mcause"));
  return 0;
}
