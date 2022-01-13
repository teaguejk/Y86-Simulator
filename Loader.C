/**
 * Names:
 * Team:
*/
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>

#include "Loader.h"
#include "Memory.h"
#include "Tools.h"

//first column in file is assumed to be 0
#define ADDRBEGIN 2   //starting column of 3 digit hex address 
#define ADDREND 4     //ending column of 3 digit hex address
#define DATABEGIN 7   //starting column of data bytes
#define COMMENT 28    //location of the '|' character 

static int lineNumber = 0;
static unsigned long oldAddress = 0x0;
static int addressInc = 0;
static unsigned long lastWritten = 0x0;

bool canOpen(int argc, char * file)
{
   //make sure the number of arguments is correct
   if(argc != 2) return false;
   
   //convert to string for ease-of-use
   std::string name = file;

   //make sure the file ends in .yo
   if(name.substr(name.length() - 3, 3).compare(".yo" ) != 0) return false;

   //make sure the file can be opened (i.e. it exists)
   if(FILE *file = fopen(name.c_str(), "r"))
   {
       fclose(file);
       return true;
   }
   else return false;
}

unsigned long convert(std::string line, int start, int end)
{
    std::string data = "";
    for(int i = start; i <= end; i++)
    {
        data += line.c_str()[i];
    }

    return std::stoul(data, NULL, 16);
    //std::string temp = line.substr(start, end - start);
    //return std::stoul(temp, NULL, 16);
}

//returns the proper length of the instruction in terms of characters.
unsigned int instructionLength(char code)
{
    switch(code)
    {
        case '0':
        case '1':
        case '9': return 2;
            break;
        case '2':
        case '6':
        case 'A':
        case 'B': return 4;
            break;
        case '7':
        case '8': return 18;
            break;
        case '3':
        case '4':
        case '5': return 20;
            break;
        default: return -1;
    }
}


bool hasData(std::string in) {
    bool found = false;
    for (int i = DATABEGIN; i < COMMENT; i++) {
        if (!isblank(in.c_str()[i])) {
          found = true;
        }
    }
    return found;
}

bool addressError(std::string current)
{
    //check first that the first two columns are 0x
    if(current.c_str()[0] == '0' && current.c_str()[1] == 'x' && current.c_str()[ADDREND+1] == ':' && current.c_str()[ADDREND+2] == ' ' )
    {
        //assuming you made it this far, now ensure that the address
        //data is formatted correctly
        for(int i = ADDRBEGIN; i <= ADDREND; i++)
        {
            if(!isxdigit(current.c_str()[i])) return true;
        }

        unsigned long address = convert(current, ADDRBEGIN, ADDREND);
        //if (oldAddress == 0x0) {
        //    goto next;
        //}

        if (addressInc == 0) {
            addressInc++;
            oldAddress = address;
            return false;
        }
        if (((address) < (lastWritten))) {
            return true;
        }

        oldAddress = address;

        return false;
    } else { 
        return true;
    }
}

bool dataError(std::string current)
{
      //if the line has no data, there is no possible error   
      
      if (!hasData(current)) {
          return false;
      }

      //get the number of characters that should be processed
      //int length = instructionLength(current.c_str()[0]);
      //if(length == -1) return true;
  
      std::string tmp = current.substr(DATABEGIN, COMMENT);
      int end = tmp.find_first_of(" ");
      std::string tmp1 = tmp.substr(0, end);
      int length = end - DATABEGIN;
      

      if ((tmp1.c_str()[0] == '1' && tmp1.c_str()[1] == '0') && tmp1.c_str()[2] == '\000') return false;
      if ((tmp1.c_str()[0] == '0' && tmp1.c_str()[1] == '0') && tmp1.c_str()[2] == '\000') return false;

      unsigned long address = convert(current, ADDRBEGIN, ADDREND);

      //if (length == 1) return true;

      if ((address + length) > 0x1000) return true;

      if (length % 2 == 0) return true;

        bool foundSpace = false;
    	for(int i = DATABEGIN; i < COMMENT; i++)
    	{
    	    if((!isxdigit(current.c_str()[i]) && current.c_str()[i] != ' ') || (isxdigit(current.c_str()[i]) && foundSpace == true))
    	    {
    	         return true;
    	    }
    	    if(current.c_str()[i] == ' ') foundSpace = true;
        }      

        return false;
}

bool hasErrors(std::string current) {

    if (current.c_str()[0] == ' ' && current.c_str()[COMMENT] == '|' && !hasData(current)) return false;
    if (current.c_str()[COMMENT] != '|') return true;
    if (addressError(current)) return true; 
    if (dataError(current)) return true;
   
    return false;

}

void loadline(std::string current) {

    lineNumber++;
    if(hasErrors(current))
    {
        std::cout << "Error on line " << std::dec << lineNumber
            << ": " << current << std::endl;
        return;
    }

    //get address

    long unsigned int address = convert(current, ADDRBEGIN, ADDREND);
    //uint32_t address = convert(current, ADDRBEGIN, ADDREND);

    //get data
    //std::string tmp = current.substr(DATABEGIN, COMMENT);
    //int end = tmp.find_first_of(" ");
    //std::string tmp1 = tmp.substr(0, end);
    //unsigned long data = convert(tmp1, 0,  end);
    
    //testing address and data
    //printf("address:  0x%03lx  data:  0x%x\n", address, data);
    
    //indexes
    int byte0 = DATABEGIN;
    int byte1 = DATABEGIN + 1;

    uint8_t data_first;
    //uint8_t data_second;

    bool imem_error = false;
    
    while(isxdigit(current.c_str()[byte0]) && isxdigit(current.c_str()[byte1]))
    {
        //get two bytes from the current line
        data_first = convert(current, byte0, byte1);
        //data_second = convert(current, byte1, byte1);
        
        //increment the indexes
        byte0 += 2;
        byte1 += 2;
        
        //printf("address: %lx data: %x %x  \n", address, data_first, data_second);
        //printf("address: %lx data: %x \n", address, data_first);

        //load bytes into memory
        Memory::getInstance()->putByte(data_first, address, imem_error);
        //Memory::getInstance()->putByte(data_second, address, imem_error);
        address += 1;
        lastWritten = address;

    }
    //printf("-----------------------------------------------------\n");
    /** code for putting data into memory
    //getting length of data
    std::string len = tmp.substr(0, tmp.find_first_of(" "));
    int length = len.length()/2;
    //printf("length %d\n ", length);

    //mem error bool needed for next steps
    bool imem_error = false;

    //loop to put data into mem one byte at a time
    for (int i = 0; i < length; i++) {
        Memory::getInstance()->putByte(Tools::getByte(data, i), address, imem_error);
        //printf("byte %d = %lx\n", i, Tools::getByte(data, i));
    }
    **/
}



/**
 * Loader constructor
 * Opens the .yo file named in the command line arguments, reads the contents of the file
 * line by line and loads the program into memory.  If no file is given or the file doesn't
 * exist or the file doesn't end with a .yo extension or the .yo file contains errors then
 * loaded is set to false.  Otherwise loaded is set to true. Dr. Wilkes was here.
 *
 * @param argc is the number of command line arguments passed to the main; should
 *        be 2
 * @param argv[0] is the name of the executable
 *        argv[1] is the name of the .yo file
 */
Loader::Loader(int argc, char * argv[])
{
   loaded = false;

   //if the number of arguments is wrong, the file is not named correctly, or the file does
   //not exist, return immediatley
   
   //Start by writing a method that opens the file (checks whether it ends 
   //with a .yo and whether the file successfully opens; if not, return without 
   //loading)
   //The file handle is declared in Loader.h.  You should use that and
   //not declare another one in this file.
   
   //Next write a simple loop that reads the file line by line and prints it out
   
   //Next, add a method that will write the data in the line to memory 
   //(call that from within your loop)

   //Finally, add code to check for errors in the input line.
   //When your code finds an error, you need to print an error message and return.
   //Since your output has to be identical to your instructor's, use this cout to print the
   //error message.  Change the variable names if you use different ones.
   //  std::cout << "Error on line " << std::dec << lineNumber
   //       << ": " << line << std::endl;

   if(!canOpen(argc, argv[1])) return;

   std::string name = argv[1];

   //open the file for reading
   inf.open(argv[1], std::ifstream::in);
   
   //iterate through the file 

   int lineNumber = 1;
   std::string current; //the current line
   while(std::getline(inf, current))
   {

      //check errors
      if (hasErrors(current)) {
        std::cout << "Error on line " << std::dec << lineNumber << ": " << current << std::endl;
        //std::cout << "Load error." << std::endl;
        //std::cout << "Usage: lab6 <file.yo>" << std::endl;
        lineNumber++;
        return;
      } 
      
        
      //loadline
      try {
        loadline(current);
      } catch (...) {
        lineNumber++;
        continue;
      } 
        
      //increment linNumber for next line
      lineNumber++;
   }

   //If control reaches here then no error was found and the program
   //was loaded into memory.
   loaded = true;   
}



/**
 * isLoaded
 * returns the value of the loaded data member; loaded is set by the constructor
 *
 * @return value of loaded (true or false)
 */
bool Loader::isLoaded()
{
   return loaded;
}


//You'll need to add more helper methods to this file.  Don't put all of your code in the
//Loader constructor.  When you add a method here, add the prototype to Loader.h in the private
//section.
