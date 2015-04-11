#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>

#include <cstring>
#include <string>
#include <stdio.h>
#include <vector>

#include <iostream>

using namespace std;

int alive = 1;

string curWD = "this is the current working directory";
string* history = new string[10];

void ResetCanonicalMode(int fd, struct termios *savedattributes)
{
  tcsetattr(fd, TCSANOW, savedattributes);
}

void SetNonCanonicalMode(int fd, struct termios *savedattributes)
{
  struct termios TermAttributes;
  char *name;
  
  // Make sure stdin is a terminal. 
  if(!isatty(fd))
  {
    fprintf (stderr, "Not a terminal.\n");
    exit(0);
  }
    
  // Save the terminal attributes so we can restore them later. 
  tcgetattr(fd, savedattributes);
  
  // Set the funny terminal modes. 
  tcgetattr (fd, &TermAttributes);
  TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO. 
  TermAttributes.c_cc[VMIN] = 1;
  TermAttributes.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}
 
vector<string> splitString(string str)
{
  vector<string> parts;
  string tok;
  int front = 0;
  int split = 0;
  
  while(1)
  {
    split = str.find(' ', front);
    if(split != -1)
    {
      tok = str.substr(front, split - front);
      front = str.find_first_not_of(' ', split);
      parts.push_back(tok);
    }
    else
    {
      tok = str.substr(front, str.size());
      parts.push_back(tok);
      break;
    }
  }
  
  return parts;
}

void parseInput(string line_in, int des)
{
  vector<string> parts = splitString(line_in);
  char *args[parts.size() + 1];
  
  for(int i = 0; i < parts.size(); i++)
  {
    args[i] = new char [parts[i].size()+1];
    strcpy(args[i], parts[i].c_str());
  }
  args[parts.size()] = NULL;
  
  if(strcmp(args[0], "cd") == 0)
  {
    
  } 
  else if(strcmp(args[0], "ls") == 0)
  {
    
  }
  else if(strcmp(args[0], "pwd") == 0)
  {
    write(des, curWD.c_str(), curWD.length());
    write(des, "\n", 1);
  }
  else if(strcmp(args[0], "history") == 0)
  {
    
  }
  else if(strcmp(args[0], "exit") == 0)
  {
    alive = 0;
    return;
  } 
  else 
  {
    //temporary printing of func plus args
    cout << "outputting \n";
    for (int i = 0; i < parts.size(); i++)
    {
      write(des, parts[i].c_str(), parts[i].length());
      write(des, "\n", 1);
    }
  }
}
 
int main(int argc, char **argv)
{
  struct termios SavedTermAttributes;
  SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
  
  int prPrompt = 1;
  int output_loc = STDOUT_FILENO;
  int input_loc = STDIN_FILENO;
  
  string prompt = "> ";
  string line_in = "";
  char char_in;
  
  do
  {
    if(prPrompt == 1)
    {
      write(output_loc, prompt.c_str(), prompt.length());
      prPrompt = 0;
    }
    read(input_loc, &char_in, 1);
    switch(char_in)
    {
      case 0x09:
        write(output_loc, "this is a test line", 19);
        break;
      //enter pressed
      case 0x0A:
        write(output_loc, "\n",2);
        parseInput(line_in, output_loc);
        line_in = "";
        prPrompt = 1;
        break;
      //backspace pressed
      case 0x7F:
        write(output_loc, "\b \b", 3);
        line_in.erase(line_in.length() - 1);
        break;
      //normal character
      default:
        write(output_loc, &char_in, 1);
        line_in += char_in;
        break;
    }
  } while (alive == 1);
  
  ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
  return 0;
}