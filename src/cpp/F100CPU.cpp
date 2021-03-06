#include "F100CPU.h"

// For Opcodes with a single instruction can pick mnemonics from this array
static string mnemonics[] = {
  "HALT/BIT/SHIFT/CONDJMP", "SJM", "CAL", "RTN/RTC",
  "STO","ADS","SBS","ICZ",
  "LDA","ADD","SUB","CMP",
  "AND","NEQ","NA","JMP"
};


F100_class::F100_class(int as, int tron) {
    adsel = as;
    for (int i=0 ; i< MEM_SIZE ; i++ ) {
      mem[i] = 0x0000;
      accessed[i]=0;
    }
    ACC=0;
    PC=0;
    OR=0;
    traceon=tron;
    reset();
  }

int F100_class::mem_read(int address) {
    if (traceon == 1) {
      printf ("READ 0x%04X 0x%04X\n", address & 0xFFFF, mem[address] & 0xFFFF);
    }
    // Indirect read to allow for decoding peripherals in memory space
    return mem[address] & 0xFFFF;
  }

void F100_class::mem_write(int address, int data) {
    if (traceon == 1){
      printf ("STORE 0x%04X 0x%04X\n", address & 0xFFFF, data & 0xFFFF);
    }
    // Indirect write to allow for decoding peripherals in memory space
    mem[address] = data & 0xFFFF;
    accessed[address]=1;
  }

void F100_class::interrupt(int channel) {
    channel = channel & 0x3F;
    lsp = mem_read(0);
    // save next PC (already incremented)
    mem_write(lsp+1, PC);
    mem_write(lsp+2, CC.pack());
    mem_write(0, lsp+2);
    // disable further interrupts
    CC.I = 1;
    // compute new PC address
    unsigned int destination = (2*channel) + ( (adsel == 1) ? 2048 : 16384);
    PC = destination & 0x7FFF;
  }

void F100_class::reset() {
    CC.reset();
    PC = (adsel==1) ? 0x0800 : 0x2000 ;
  }

int F100_class::single_step(){
    unsigned int operand;
    unsigned int operand_adr;
    unsigned int operand2;
    unsigned int retval = 0;
    unsigned int alu_result;

    IR.unpack( mem_read(PC++) );
    instr_disassembly = mnemonics[IR.F];
    switch (IR.F) {
    case 0x0:
      if (IR.T == 1) { // HALT
        instr_disassembly="HALT";
        retval = IR.IR & 0x03FF;
      } else if (IR.S==2) { // JUMP CONDITIONAL
        unsigned int bitmask = 0x01 << IR.B;
        unsigned int W, W1;
        if (IR.R == 3){
          W = mem_read(PC++);
          operand = mem_read(W);
          W1 = mem_read(PC++);
        } else {
          if (IR.R == 1){
            operand = CC.pack();
          } else {
            operand = ACC;
          }
          W1 = mem_read(PC++);
        }

        if (IR.J == 0 || IR.J == 2){ // JBC, JCS
          instr_disassembly="JBC";
          if ((operand & bitmask) == 0){
            PC = W1 & 0x7FFF;
            if (IR.J == 2) {
              instr_disassembly="JCS";
              if (IR.R == 3) {
                mem_write(W, operand | bitmask);
              } else if (IR.R == 1) {
                CC.unpack(operand | bitmask);
              } else {
                ACC = operand | bitmask;
              }
            }
          }
        } else { // JBS, JSC
          instr_disassembly="JBS";
          if ((operand & bitmask) != 0) {
            PC = W1 ;
            if (IR.J == 3) {
              instr_disassembly="JSC";
              if (IR.R == 3){
                mem_write(W, operand &  ~bitmask);
              } else if (IR.R == 1) {
                CC.unpack(operand & ~bitmask);
              } else {
                ACC = operand & ~bitmask;
              }
            }
          }
        }
      } else if (IR.S==3) { // CLR,SET
        unsigned int bitmask = 0x01 << IR.B;
        if (IR.J == 3){ // CLR
            instr_disassembly="CLR";
            if (IR.R == 3){
                operand_adr = mem_read(PC++) ;
                alu_result = mem_read(operand_adr) & ~bitmask;
                mem_write(operand_adr, alu_result);
            } else if (IR.R == 1) {
                CC.unpack(CC.pack() & ~bitmask);
            } else {
                ACC = ACC & ~bitmask;
            }
        } else if (IR.J == 2){ //SET
            instr_disassembly="SET";
            if (IR.R == 3) {
                operand_adr = mem_read(PC++);
                alu_result = mem_read(operand_adr) | bitmask;
                mem_write(operand_adr, alu_result);
            } else if (IR.R == 1){
                CC.unpack(CC.pack() | bitmask);
            } else {
                ACC = ACC | bitmask;
            }
        } else {
          // illegal opcode - ignore
        }
      } else { // SHIFT
        int overflow;
        int sign;
        int i;
        if (CC.M == 0) { // Single length shifts and rotates
          if (IR.R == 1) {
            operand = CC.pack();
          } else if (IR.R == 3) {
            operand_adr = mem_read(PC++);
            operand = mem_read(operand_adr);
          } else {
            operand = ACC;
          }
          overflow = 0;
          alu_result = operand & 0xFFFF;

          if ((IR.S == 0) && (IR.J <2)) { // SRA
            instr_disassembly="SRA";
            sign = operand & 0x8000;
            for (i=0; i<IR.B ; i++){
              alu_result = (alu_result >> 1) | sign;
            }
            CC.S = (alu_result & 0x8000) ? 1:0;
            CC.V = (alu_result & 0x8000) != (operand & 0x8000) ? 1:0;
          } else if ((IR.S == 0 ) && (IR.J == 2)) { //SRL
            instr_disassembly="SRL";
            for (i=0; i<IR.B ; i++){
              alu_result = (alu_result >> 1) & 0x7FFF;
            }
          } else if ((IR.S == 0) & (IR.J == 3)){ // ROTR
            instr_disassembly="SRE";
            for (i=0; i<IR.B ; i++){
              int wrap = (alu_result & 0x1) << 15;
              alu_result = (alu_result >> 1) | wrap;
            }
          } else if (( IR.S == 1) && (IR.J != 3)) { // SLL,SLA
            instr_disassembly="SLL";
            for (i=0; i<IR.B ; i++){
              alu_result = (alu_result << 1) & 0xFFFF;
              if ((alu_result & 0x8000) != (operand & 0x8000)){
                overflow = 1;
              }
            }
            if (IR.J != 2) {
              instr_disassembly="SLA";
              CC.S = (alu_result & 0x8000) ? 1:0;
              CC.V = overflow;
            }
          } else if ((IR.S == 1) && (IR.J == 3)) { // ROTL
            instr_disassembly="SLE";
            for (i=0; i<IR.B ; i++){
              int wrap = (alu_result & 0x8000) >> 15;
              alu_result = ((alu_result << 1) & 0xFFFE) | wrap;
            }
          }

          if (IR.R == 1){
              CC.unpack(alu_result);
          } else if (IR.R ==3){
              mem_write(operand_adr, alu_result);
          } else {
            ACC = alu_result;
          }
        } else { // Double length shifts use LSB of J field to extend shift number
          int shift_dist;

          shift_dist = ( (IR.J << 4) | IR.B )  & 0x1F;
          overflow =  0;
          if ((IR.S == 0) && (IR.J <2)) { // SRA.D
            instr_disassembly="SRA.D";
            alu_result = ( (ACC<<16) | (OR & 0xFFFF) ) >> shift_dist;
            if (ACC & 0x8000) {
              sign = 0xFFFFFFFF << (32-shift_dist);
              alu_result |= sign;
            }
          } else if (IR.S == 0){ // SRL.D
            instr_disassembly="SRL.D";
            alu_result = (((ACC<<16) | (OR & 0xFFFF) ) & 0xFFFFFFFF) >> shift_dist;
          } else { // SLL.D, SLA.D
            if ( IR.J < 2) {
              instr_disassembly="SLA.D";
            } else {
              instr_disassembly="SLL.D";
            }
            alu_result = ( (ACC<<16) | (OR & 0xFFFF) ) ;
            for (int j=0; j<shift_dist; j++) {
              alu_result = (alu_result << 1);
              if (((alu_result & 0x80000000) >> 16) != (ACC & 0x8000)) {
                overflow = 1;
              }
            }
          }
          if (IR.J < 2) {
            CC.S = (alu_result & 0x80000000) ? 1:0;
            CC.V = overflow;
          }
        ACC = (alu_result >> 16 ) & 0xFFFF;
        OR = alu_result & 0xFFFF;
        }
      }
      break;
    case 0x1: //SJM
      // Note that the PC has already been incremented during the instruction fetch
      PC = (PC + ACC ) & 0x7FFF;
      break;
    case 0x2: //CAL
      lsp = mem_read(0);
      get_operand(&operand_adr, 1, 1);
      // save next PC (already incremented)
      // special case for Immediate addressing, must push the immediate data address on the stack
      // and then set that to be the PC also.
      if (IR.I==0 && IR.N==0) {
        PC = operand_adr & 0x7FFF;
      }
      mem_write(lsp+1, PC);
      mem_write(lsp+2, CC.pack());
      mem_write(0, lsp+2);
      PC = operand_adr & 0x7FFF;
      CC.M = 0;
      break;
    case 0x3: //RTN, RTC
      // Note that the PC has already been incremented during the instruction fetch
      lsp = mem_read(0);
      PC = mem_read(lsp-1) & 0x7FFF;
      if (IR.I == 0) {
        instr_disassembly="RTN";
        // Restore only bits 0-5 from the stack
        condition= mem_read(lsp) & 0x3F;
        unsigned int current_cr = CC.pack() & 0x0040;
        CC.unpack( (current_cr|condition) );
      } else {
        instr_disassembly="RTC";
      }
      mem_write(0, lsp-2);

      break;
    case 0x4: //STO
      get_operand(&operand_adr, 1);
      mem_write(operand_adr, ACC);
      CC.Z = ((ACC & 0xFFFF) == 0) ? 1 : 0;
      CC.S = ((ACC & 0x8000) != 0) ? 1 : 0;
      CC.V = 0;
      break;
    case 0x7: //ICZ
      OR=get_operand(&operand_adr);
      operand2 = mem_read(PC++);
      alu_result = (OR + 1) & 0xFFFF;
      if (IR.I!=0 || IR.N!=0){
        // return the incremented counter value to its original location
        mem_write(operand_adr, alu_result);
      }
      if (alu_result != 0){
        PC = operand2 & 0x7FFF;
      }
      break;
    case 0x8: //LDA
        ACC=get_operand(&operand_adr);
        CC.Z = ((ACC & 0xFFFF) == 0) ? 1:0;
        CC.S = ((ACC & 0x8000) != 0) ? 1:0;
        CC.V = 0;
        break;
    case 0x5: //ADS
    case 0x6: //SBS
    case 0x9: //ADD
    case 0xA: //SUB
    case 0xB: //CMP
    case 0xC: //AND
    case 0xD: //NEQ
      OR=get_operand(&operand_adr);
      if (IR.F==0xA || IR.F==0xB || IR.F==0x6) { // CMP, SUB, SBS
        alu_result = OR - ACC;
        if (CC.M==1){
          alu_result = alu_result + CC.C - 1;
        }
        CC.C=((alu_result & 0x010000)>0)? 0 : 1;
        CC.V=((ACC & 0x8000) != (OR & 0x8000)) && ((alu_result & 0x8000) == (ACC & 0x8000));
      } else if (IR.F==0x5 || IR.F==0x9) { // ADD, ADS
        alu_result = OR + ACC;
        if (CC.M==1){
          alu_result = alu_result + CC.C;
        }
        CC.C=((alu_result & 0x010000)>0)? 1 : 0;
        CC.V=((ACC & 0x8000) == (OR & 0x8000)) && ((alu_result & 0x8000) != (ACC & 0x8000));
      } else if (IR.F==0xC) { //AND
        alu_result = ACC & OR;
        CC.C=1;
      } else { //NEQ
        alu_result = ACC ^ OR;
        CC.C=1;
      }
      CC.Z=((alu_result&0xFFFF)==0) ? 1 : 0;
      CC.S=((alu_result&0x8000)!=0) ? 1 : 0;
      if (IR.F<0x7) { // ADS, SBS
        mem_write(operand_adr,alu_result);
      } else if (IR.F != 0xB ){ // Everything except CMP
        ACC = alu_result;
      }
      break;
    case 0xF: //JMP
      get_operand(&operand_adr, 1);
      PC = operand_adr;
      break;
    }
    return retval;
  }

int F100_class::get_operand(unsigned int *operand_adr, unsigned int noread, unsigned int nopointerarith){
    unsigned int operand = 0;
    if (IR.I==0 && IR.N==0) { //Immediate addressing
      *operand_adr = PC++;
    } else if (IR.I==0) { //Direct addressing
      *operand_adr=IR.N;
    } else if (IR.I==1 && IR.P==0) {//Immediate Indirect
      *operand_adr=mem_read(PC++);
    } else { //Pointer Indirect
      int pointer_val=mem_read(IR.P);
      if (nopointerarith==1 || (IR.R & 0x1)==0 ){
        *operand_adr = pointer_val;
      } else{
        pointer_val+= (IR.R==1)?1:0; //Pointer pre-increment
        *operand_adr = pointer_val;
        pointer_val-= (IR.R==3)?1:0; //Pointer post-increment
        mem_write(IR.P, pointer_val);
      }
    }
    if (noread==0) {
      operand=mem_read(*operand_adr);
    }
    return operand;
  }

