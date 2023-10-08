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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define Reg(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write
#define Rw register_addi
#define Jal jump_jal
#define Jalr jump_jalr
#define SetIfCon cmp_and_return
#define Branch branch
#define Divw div_divw
#define Ecall ecall_helper
#define Csrrs isa_csrrs
#define Csrrw isa_csrrw

word_t register_addi(word_t imm, int idx);
word_t jump_jal(int32_t imm, Decode *s, int dest);
word_t jump_jalr(int32_t imm, Decode *s, uint32_t rs1, int dest);
word_t cmp_and_return(uint32_t src1, uint32_t imm, int type);
void branch(uint32_t src1, uint32_t src2, uint32_t imm, Decode *s, int type);
word_t div_divw(word_t src1, uint32_t src2);
word_t isa_csrrs(word_t dest, word_t src1, word_t csr);
word_t isa_csrrw(word_t dest, word_t src1, word_t csr);
void ecall_helper(Decode *s);


extern word_t isa_raise_intr(word_t NO, vaddr_t epc);
// extern riscv32_CSR_state CSRs;

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_J, TYPE_R,  TYPE_B,
  TYPE_N, // none
};

enum {
  Beq = 512, Bge, Bgeu, Blt, Bltu, Bne, Sltu, Sltiu, Slt, Slti 
} Instruction_type;

#define src1R() do { *src1 = Reg(rs1); } while (0)
#define src2R() do { *src2 = Reg(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
// #define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 19) | (BITS(i, 19, 12) << 11) | (BITS(i, 20, 20) << 10) | (BITS(i, 30, 21)); } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 30,21) << 1) |(BITS(i,20,20) << 11) |(BITS(i,19,12)<<12);} while(0)
// #define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 11) | (BITS(i, 7, 7) << 10) | (BITS(i, 30, 25) << 4) | (BITS(i, 11, 8)); } while(0)
#define immB() do {*imm = (SEXT(BITS(i,31,31),1)<<12)| ((BITS(i, 11,8))<<1)|((BITS(i,7,7))<<11)|((BITS(i,30,25))<<5);} while(0)
static void decode_operand(Decode *s, int *dest, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rd  = BITS(i, 11, 7);
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *dest = rd;
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_B: src1R(); src2R(); immB(); break;
  }
}


static int decode_exec(Decode *s) {
  int dest = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &dest, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, Reg(dest) = SEXT(BITS(imm, 31, 12) << 12, 32));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, Reg(dest) = SEXT(Mr(src1 + imm, 4), 32));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, BITS(src2, 31, 0)));
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, Reg(10)));                          // Reg(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, Reg(dest) = s->pc + imm);
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, Reg(dest) = src1 +src2);
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, Reg(dest) = src1 + imm);
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, Reg(dest) = Jal(imm, s, dest));                     
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, Reg(dest) = Jalr(imm, s, src1, dest));                    
  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd     , S, Mw(src1 + imm, 8, src2));
  INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw   , R, Reg(dest) = SEXT(BITS(src1 + src2, 31, 0), 32));
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw   , R, Reg(dest) = SEXT(BITS(src1 - src2, 31, 0), 32));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, Reg(dest) = src1 - src2);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, Reg(dest) = SetIfCon(src1, imm, Sltiu));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, Reg(dest) = SetIfCon(src1, src2, Slt));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, Reg(dest) = SetIfCon(src1, imm, Slti));
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, Branch(src1, src2, imm, s, Bne));
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, Branch(src1, src2, imm, s, Beq));
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, Branch(src1, src2, imm, s, Bge));
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, Branch(src1, src2, imm, s, Blt));
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, Branch(src1, src2, imm, s, Bgeu));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, Branch(src1, src2, imm, s, Bltu));
  INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw  , I, Reg(dest) = SEXT(BITS(src1 + imm, 31, 0), 32)); 
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld     , I, Reg(dest) = Mr(src1 + imm, 8));
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, Reg(dest) = SEXT(BITS(Mr(src1 + imm, 1), 7, 0), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, Reg(dest) = SEXT(BITS(Mr(src1 + imm, 2), 15, 0), 16)); // 64 or 16 ?
  INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu    , I, Reg(dest) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, Reg(dest) = Mr(src1 + imm, 2));  
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, Reg(dest) = src1 << BITS(imm, 5, 0));
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli   , I, Reg(dest) = src1 >> BITS(imm, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw  , I, Reg(dest) = SEXT(BITS(src1, 31, 0) >> BITS(imm, 4, 0), 32));        // not diffset
  // INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw   , R, Reg(dest) = SEXT(BITS(src1 * src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, int64_t result =((int64_t)src1 * (int64_t)src2);Reg(dest) = (uint32_t)(result>>32));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, BITS(src2, 31, 0)));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, BITS(src2, 7, 0)));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, BITS(src2, 15, 0)));
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, Reg(dest) = (int32_t)src1 / (int32_t)src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw   , R, Reg(dest) = SEXT((int32_t)BITS(src1, 31, 0) / (int32_t)BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw  , R, Reg(dest) = BITS(src1, 31, 0) / BITS(src2, 31, 0));
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, Reg(dest) = Reg(dest) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw   , R, Reg(dest) = SEXT((int32_t)BITS(src1, 31, 0) % (int32_t)BITS(src2, 31, 0), 32));
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai   , I, Reg(dest) = (int32_t)src1 >> BITS(imm, 5, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, Reg(dest) = (int32_t)src1 >> ((int32_t)src2&0b11111) );
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, Reg(dest) = Mr(src1 + imm, 1));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, Reg(dest) = src1 << BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, Reg(dest) = src1 >> BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw   , R, Reg(dest) = SEXT(BITS(src1, 31, 0) << BITS(src2, 5, 0), 32));
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, Reg(dest) = src1 & src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, Reg(dest) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, Reg(dest) = src1 | src2);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, Reg(dest) = src1 | imm);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, Reg(dest) = SetIfCon(src1, src2, Sltu));
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, Reg(dest) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, Reg(dest) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw  , I, Reg(dest) = SEXT(BITS(src1, 31, 0) << BITS(imm, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw  , I, Reg(dest) = SEXT((int32_t)(BITS(src1, 31, 0)) >> BITS(imm, 4, 0), 32));
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw   , R, Reg(dest) = SEXT(BITS(src1, 31, 0) << BITS(src2, 4, 0), 32));
  INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw   , R, Reg(dest) = SEXT(BITS(src1, 31, 0) >> BITS(src2, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw   , R, Reg(dest) = SEXT((int32_t)(BITS(src1, 31, 0)) >> BITS(src2, 4, 0), 32));
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, Reg(dest) = src1 * src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, Reg(dest) = src1 % src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw  , R, Reg(dest) = SEXT(BITS(src1, 31, 0) % BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, Reg(dest) = (int32_t)src1 % (int32_t)src2);
  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I, Reg(dest) = Csrrs(dest, src1, imm));
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I, Reg(dest) = Csrrw(dest, src1, imm));
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , I, Ecall(s));
  // INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , R, s->dnpc = CSRs.mepc);

  //       0000000 01110 01111 010 00000 01000 11 
  //          0      14    15   2   0      8   3
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc)); 
  INSTPAT_END();

  Reg(0) = 0; // reset $zero to 0

  return 0;
}
void trace_inst(word_t pc, uint32_t inst);
int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  IFDEF(CONFIG_ITRACE, trace_inst(s->pc, s->isa.inst.val));
  return decode_exec(s);
}


word_t register_addi(word_t imm, int idx) {
  return cpu.gpr[check_reg_idx(idx)] + imm;
}
void trace_func_call(paddr_t pc, paddr_t target);
void trace_func_ret(paddr_t pc);
word_t jump_jal(int32_t imm, Decode *s, int dest) {
  s->dnpc = cpu.pc + imm;
  #ifdef CONFIG_FTRACE_COND
  int is_ret = -1;
  char *func_name = get_func_name(s->snpc - 4, s->dnpc, &is_ret);
  if (func_name) {
    ftrace_info temp = {func_name, s->dnpc, is_ret, s->snpc - 4};
    call_ret_table[ftrace_index++] = temp;
  }

  #endif
  IFDEF(CONFIG_ITRACE, { 
  if (dest == 1) { // x1: return address for jumps
    trace_func_call(s->pc, s->dnpc);
  }
  })
  return cpu.pc+4;
}

word_t jump_jalr(int32_t imm, Decode *s, uint32_t src1, int dest) {
  // s->dnpc = (2 * imm + src1) & (~1);
  s->dnpc = (src1 + imm) & (~1);
  #ifdef CONFIG_FTRACE_COND
  int is_ret = -1;
    char *func_name = get_func_name(s->snpc - 4, s->dnpc, &is_ret);
    if (func_name) {
      assert(is_ret != -1);
      ftrace_info temp = {func_name, s->dnpc, is_ret, s->snpc - 4};
      call_ret_table[ftrace_index++] = temp;
    }
  #endif
  IFDEF(CONFIG_ITRACE, {
  if (s->isa.inst.val == 0x00008067) {
    trace_func_ret(s->pc); // ret -> jalr x0, 0(x1)
  } else if (dest == 1) {
    trace_func_call(s->pc, s->dnpc);
  } else if (dest == 0 && imm == 0) {
    trace_func_call(s->pc, s->dnpc); // jr rs1 -> jalr x0, 0(rs1), which may be other control flow e.g. 'goto','for'
  }
  })
  printf("FROM PC %02x JALR to PC:%02x\n",cpu.pc,s->dnpc);
  return s->snpc;
}

word_t cmp_and_return(word_t num1, word_t num2, int type) {
  switch (type)
  {
  case Sltu:
    if (num1 < num2) return 1;
      return 0;
    break;
  case Sltiu:
    if (num1 < num2) return 1;
      return 0;
  case Slt:
    if ((int32_t)num1 < (int32_t)num2) return 1;
      return 0;
    break;
  case Slti:
    if ((int32_t)num1 < (int32_t)num2) return 1;
      return 0;
    break;
  default:
    panic("cmp_and_return fuction falut!");
    break;
  }
  return 0;
}


void branch(word_t src1, word_t src2, word_t imm, Decode *s, int type) {
  switch (type) {
    case Beq: 
      if (src1 == src2) {
        s->dnpc += imm - 4;
      }
      break;
    case Bne:
      if (src1 != src2) {
        s->dnpc += imm - 4;
      }
      break;
    case Bge:
      if ((int32_t)src1 >= (int32_t)src2) {
        s->dnpc += imm - 4;
      }
      break;
    case Blt:
      if ((int32_t)src1 < (int32_t)src2) {
        s->dnpc += imm - 4;
      }
      break;
    case Bltu:
      if (src1 < src2) {
        s->dnpc += imm - 4;
      }
      break;
    case Bgeu:
      if (src1 >= src2) {
        s->dnpc += imm - 4;
      }
      break;
    default:
      break;
  }
}

word_t div_divw(word_t src1, word_t src2) {
  int32_t divisor = BITS(src1, 31, 0);
  int32_t dividend = BITS(src2, 31, 0);
  if (dividend == 0) {
    return 0;
  }
  return SEXT(divisor / dividend, 32);
}


word_t isa_csrrs(word_t dest, word_t src1, word_t csr) {
  word_t t = 0;
  switch (csr) {
    case 0x341:   // mepc
      // t = CSRs.mepc;
      t = cpu.csr.mepc;
      cpu.csr.mepc = src1 | t;
      break;
    case 0x342:   // mcause
      t = cpu.csr.mcause;
      cpu.csr.mcause = src1 | t;
      break;
    case 0x300:   // mstatus
      t = cpu.csr.mstatus;
      cpu.csr.mstatus = src1 | t;
      break;
    case 0x305:   // mtvec
      t = cpu.csr.mtvec;
      cpu.csr.mtvec = src1 | t;
      break;
    default:
      panic("Unkown csr!");
      break;
  }
// printf("csr: %lx, t: %lx, src1: %lx, dest: %lx, mepc: %lx, mcause: %lx, mstatus: %lx, mtvec: %lx\n", csr, t, src1, dest, CSRs.mepc, CSRs.mcause, CSRs.mstatus, CSRs.mtvec);
  return t;
}

word_t isa_csrrw(word_t dest, word_t src1, word_t csr) {
  word_t t = 0;
  switch (csr) {
    case 0x341:   // mepc
      t = cpu.csr.mepc;
      cpu.csr.mepc = src1;
      break;
    case 0x342:   // mcause
      t = cpu.csr.mcause;
      cpu.csr.mcause = src1;
      break;
    case 0x300:   // mstatus
      t = cpu.csr.mstatus;
      cpu.csr.mstatus = src1;
      break;
    case 0x305:   // mtvec
      t = cpu.csr.mtvec;
      cpu.csr.mtvec = src1;
      break;
    default:
      panic("Unkown csr!");
      break;
  }
  return t;
}


void ecall_helper(Decode *s) {

  s->dnpc = isa_raise_intr(0xb, cpu.pc);
  // printf("ECALL from PC %02lx to PC:%02lx\n", cpu.pc, s->dnpc);
}



