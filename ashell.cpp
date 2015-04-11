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

char* curWD = new char[PATH_MAX];
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
  int front;
  int split;
  
  // trims leading and trailing whitespace
  front = str.find_first_not_of(' ');
  split = str.find_last_not_of(' ');
  str = str.substr(front, split - front + 1);
  
  front = 0;
  split = 0;
  
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

void parseInput(string line_in)
{
  if(line_in.empty())
  {
    return;
  }
  
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
    write(STDOUT_FILENO, curWD, strlen(curWD));
    write(STDOUT_FILENO, "\n", 1);
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
      write(STDOUT_FILENO, parts[i].c_str(), parts[i].length());
      write(STDOUT_FILENO, "\n", 1);
    }
  }
}
 
int main(int argc, char **argv)
{
  struct termios SavedTermAttributes;
  SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
  
  int prPrompt = 1;  
  string prompt = "> ";
  string line_in = "";
  char char_in;
  
  getcwd(curWD, PATH_MAX);
  
  do
  {
    if(prPrompt == 1)
    {
      string temp = curWD;
      if(temp.length()>16)
      {
        int slash = temp.rfind('/');
        temp = temp.substr(slash,temp.length());
        temp = "/..." + temp;
      }
      write(STDOUT_FILENO, temp.c_str(), temp.length());
      write(STDOUT_FILENO, prompt.c_str(), prompt.length());
      prPrompt = 0;
    }
    read(STDIN_FILENO, &char_in, 1);
    switch(char_in)
    {
      //enter pressed
      case 0x0A:
        write(STDOUT_FILENO, "\n",2);
        parseInput(line_in);
        line_in = "";
        prPrompt = 1;
        break;
      //backspace pressed
      case 0x7F:
        write(STDOUT_FILENO, "\b \b", 3);
        line_in.erase(line_in.length() - 1);
        break;
      //normal character
      default:
        write(STDOUT_FILENO, &char_in, 1);
        line_in += char_in;
        break;
    }
  } while (alive == 1);
  
  ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
  return 0;
}