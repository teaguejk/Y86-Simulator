//class to perform the combinational logic of
//the Decode stage
class DecodeStage: public Stage
{
   private:
      void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                            uint64_t valC, uint64_t valA, uint64_t valB,
                            uint64_t dstE, uint64_t dstM,
                            uint64_t srcA, uint64_t srcB);
      unsigned long srcAComponent(unsigned long icode, unsigned long D_rA);
      unsigned long srcBComponent(unsigned long icode, unsigned long D_rB);
      unsigned long dstEComponent(uint64_t icode, uint64_t rB);
      unsigned long dstMComponent(uint64_t icode, uint64_t rA);
      uint64_t valAComponent(uint64_t rvalA, uint64_t d_srcA, M * mreg, W * wreg, Stage * estage, Stage * mstage);
      uint64_t valBComponent(uint64_t rvalB, uint64_t d_srcB, M * mreg, W * wreg, Stage * estage, Stage * mstage);

      uint64_t srcA;
      uint64_t srcB;
      
      bool calculateControlSignals(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
      bool E_bubble;

      void normalE(PipeReg ** pregs);        
      void bubbleE(PipeReg ** pregs);

   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      
      uint64_t getdsrcA();
      uint64_t getdsrcB();

};
