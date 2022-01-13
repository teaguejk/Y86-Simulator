//class to perform the combinational logic of
//the Memory stage
class MemoryStage: public Stage
{
   public:
      uint64_t getvalM();
      uint64_t getm_stat();
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);
      void setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE,
			    uint64_t valA, uint64_t dstE, uint64_t dstM);
      uint64_t m_stat(bool mem_error, uint64_t M_stat);
   private:
     uint64_t valM;
     uint64_t stat;
};
