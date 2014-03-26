//public:
  #include "registers.hpp"
  #include "memory.hpp"
  //#include "disassembler/disassembler.hpp"

  regs_t regs;
  uint16 dp, sp, rd, wr, bit, ya;

  void op_io();
  uint8 op_read(uint16 addr);
  void op_write(uint16 addr, uint8 data);

  alwaysinline uint8  op_adc (uint8  x, uint8  y);
  alwaysinline uint16 op_addw(uint16 x, uint16 y);
  alwaysinline uint8  op_and (uint8  x, uint8  y);
  alwaysinline uint8  op_cmp (uint8  x, uint8  y);
  alwaysinline uint16 op_cmpw(uint16 x, uint16 y);
  alwaysinline uint8  op_eor (uint8  x, uint8  y);
  alwaysinline uint8  op_inc (uint8  x);
  alwaysinline uint8  op_dec (uint8  x);
  alwaysinline uint8  op_or  (uint8  x, uint8  y);
  alwaysinline uint8  op_sbc (uint8  x, uint8  y);
  alwaysinline uint16 op_subw(uint16 x, uint16 y);
  alwaysinline uint8  op_asl (uint8  x);
  alwaysinline uint8  op_lsr (uint8  x);
  alwaysinline uint8  op_rol (uint8  x);
  alwaysinline uint8  op_ror (uint8  x);

  template<int, int> alwaysinline void op_mov_reg_reg();
  alwaysinline void op_mov_sp_x();
  template<int> alwaysinline void op_mov_reg_const();
  alwaysinline void op_mov_a_ix();
  alwaysinline void op_mov_a_ixinc();
  template<int> alwaysinline void op_mov_reg_dp();
  template<int, int> alwaysinline void op_mov_reg_dpr();
  template<int> alwaysinline void op_mov_reg_addr();
  template<int> alwaysinline void op_mov_a_addrr();
  alwaysinline void op_mov_a_idpx();
  alwaysinline void op_mov_a_idpy();
  alwaysinline void op_mov_dp_dp();
  alwaysinline void op_mov_dp_const();
  alwaysinline void op_mov_ix_a();
  alwaysinline void op_mov_ixinc_a();
  template<int> alwaysinline void op_mov_dp_reg();
  template<int, int> alwaysinline void op_mov_dpr_reg();
  template<int> alwaysinline void op_mov_addr_reg();
  template<int> alwaysinline void op_mov_addrr_a();
  alwaysinline void op_mov_idpx_a();
  alwaysinline void op_mov_idpy_a();
  alwaysinline void op_movw_ya_dp();
  alwaysinline void op_movw_dp_ya();
  alwaysinline void op_mov1_c_bit();
  alwaysinline void op_mov1_bit_c();

  alwaysinline void op_bra();
  template<int, int> alwaysinline void op_branch();
  template<int, int> alwaysinline void op_bitbranch();
  alwaysinline void op_cbne_dp();
  alwaysinline void op_cbne_dpx();
  alwaysinline void op_dbnz_dp();
  alwaysinline void op_dbnz_y();
  alwaysinline void op_jmp_addr();
  alwaysinline void op_jmp_iaddrx();
  alwaysinline void op_call();
  alwaysinline void op_pcall();
  template<int> alwaysinline void op_tcall();
  alwaysinline void op_brk();
  alwaysinline void op_ret();
  alwaysinline void op_reti();

  template<uint8 (SMP::*)(uint8, uint8), int> alwaysinline void op_read_reg_const();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_a_ix();
  template<uint8 (SMP::*)(uint8, uint8), int> alwaysinline void op_read_reg_dp();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_a_dpx();
  template<uint8 (SMP::*)(uint8, uint8), int> alwaysinline void op_read_reg_addr();
  template<uint8 (SMP::*)(uint8, uint8), int> alwaysinline void op_read_a_addrr();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_a_idpx();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_a_idpy();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_ix_iy();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_dp_dp();
  template<uint8 (SMP::*)(uint8, uint8)> alwaysinline void op_read_dp_const();
  template<uint16 (SMP::*)(uint16, uint16)> alwaysinline void op_read_ya_dp();
  alwaysinline void op_cmpw_ya_dp();
  template<int> alwaysinline void op_and1_bit();
  alwaysinline void op_eor1_bit();
  alwaysinline void op_not1_bit();
  template<int> alwaysinline void op_or1_bit();

  template<uint8 (SMP::*)(uint8), int> alwaysinline void op_adjust_reg();
  template<uint8 (SMP::*)(uint8)> alwaysinline void op_adjust_dp();
  template<uint8 (SMP::*)(uint8)> alwaysinline void op_adjust_dpx();
  template<uint8 (SMP::*)(uint8)> alwaysinline void op_adjust_addr();
  template<int> alwaysinline void op_adjust_addr_a();
  template<int> alwaysinline void op_adjustw_dp();

  alwaysinline void op_nop();
  alwaysinline void op_wait();
  alwaysinline void op_xcn();
  alwaysinline void op_daa();
  alwaysinline void op_das();
  template<int, int> alwaysinline void op_setbit();
  alwaysinline void op_notc();
  template<int> alwaysinline void op_seti();
  template<int, int> alwaysinline void op_setbit_dp();
  template<int> alwaysinline void op_push_reg();
  alwaysinline void op_push_p();
  template<int> alwaysinline void op_pop_reg();
  alwaysinline void op_pop_p();
  alwaysinline void op_mul_ya();
  alwaysinline void op_div_ya_x();

  alwaysinline void do_op(uint8 opcode);

  void core_serialize(serializer&) NALL_COLD;
