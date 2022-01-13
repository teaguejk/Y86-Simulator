
class Loader
{
   private:
      bool loaded;        //set to true if a file is successfully loaded into memory
      std::ifstream inf;  //input file handle
      unsigned int instructionLength(char code);
   public:
      Loader(int argc, char * argv[]);
      bool isLoaded();
};
