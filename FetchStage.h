//class to perform the combinational logic of
//the Fetch stage
class FetchStage: public Stage
{
   private:
      void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t rA, uint64_t rB,
                     uint64_t valC, uint64_t valP);
      uint64_t selectPC(F * freg, M * mreg, W * wreg);
      bool needsRegIds(uint64_t icode);
      bool needsValC(uint64_t icode);
      //unsigned int getvalP(unsigned int pc, unsigned int icode);
      uint64_t predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP);  
      uint64_t PCincrement(int32_t f_pc, bool needsRegIds, bool needsValC);
      void getRegIds(uint64_t f_pc, uint64_t * rA, uint64_t * rB);
      void buildValC(uint64_t f_pc, uint64_t * valC, bool needsRegIds);
      bool instr_valid(uint64_t f_icode);
      uint64_t f_stat(bool mem_error, bool valid, uint64_t f_icode);
      uint64_t f_icode(bool mem_error, uint64_t f_icode);
      uint64_t f_ifun(bool mem_error, uint64_t f_ifun);
      bool F_stall;
      bool D_stall;
      bool fetch_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
      bool decode_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
      bool D_bubble;
      bool decode_bubble(uint64_t E_icode, uint64_t e_Cnd);
      void bubbleD(PipeReg ** pregs);
      void normalD(PipeReg ** pregs);

   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
