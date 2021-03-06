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

#include <iostream> //remove this.

using namespace std;

int alive = 1;

char *curWD = new char[PATH_MAX];
char *home_path = getenv("HOME");
vector<string> history;
int hist_index = 0;

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
  vector<string> tokens;
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
    split = str.find_first_of(" <>/|&", front);
    if(split != -1)
    {
      tok = str.substr(front, split - front);
      front = str.find_first_not_of(" ", split);
      split = front;
      if(strcspn(&str[split], "<>/|&") == 0)
      {
        tokens.push_back(tok);
        tok = str[split];
        front = str.find_first_not_of(" <>/|&", split);
      }
      tokens.push_back(tok);
      continue;
    }
    else if(front != -1)
    {
      tok = str.substr(front, str.size());
      tokens.push_back(tok);
      break;
    }
    else
    {
      break;
    }
  }
  return tokens;
}

void addToHistory(string str)
{
  if(str.empty())
  {
    return;
  }
  if(history.size() == 10)
  {
    history.erase(history.begin());
  }
  history.push_back(str);
}

void printPermissions(char *dir_name, char *file_name)
{
  struct stat stats;
  string full_path;
  full_path += dir_name;
  full_path += '/';
  full_path += file_name;
  
  stat(full_path.c_str(), &stats);
  
  //directory check
  if((stats.st_mode & S_IFMT) == S_IFDIR)
  {
    write(STDOUT_FILENO, "d", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  //owner permissions
  if(stats.st_mode & S_IRUSR)
  {
    write(STDOUT_FILENO, "r", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IWUSR)
  {
    write(STDOUT_FILENO, "w", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IXUSR)
  {
    write(STDOUT_FILENO, "x", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  //group permissions
  if(stats.st_mode & S_IRGRP)
  {
    write(STDOUT_FILENO, "r", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IWGRP)
  {
    write(STDOUT_FILENO, "w", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IXGRP)
  {
    write(STDOUT_FILENO, "x", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  //other permissions
  if(stats.st_mode & S_IROTH)
  {
    write(STDOUT_FILENO, "r", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IWOTH)
  {
    write(STDOUT_FILENO, "w", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  if(stats.st_mode & S_IXOTH)
  {
    write(STDOUT_FILENO, "x", 1);
  }
  else
  {
    write(STDOUT_FILENO, "-", 1);
  }
  
  write(STDOUT_FILENO, " ", 1);
}

char ** getArgs(vector<string> str)
{
  char **args = new char*[str.size() + 1];
  for(int i = 0; i < str.size(); i++)
  {
    args[i] = new char [str[i].size()+1];
    strcpy(args[i], str[i].c_str());
  }
  args[str.size()] = NULL;
  
  return args;
}

void parseInput(string line_in)
{
  if(line_in.empty())
  {
    return;
  }

  int redir_out = 0;
  int redir_in = 0;
  
  string out_loc = "";
  string in_loc = "";
  
  int num_pipes = 0;
  int previnpipe = -1;
  int inpipe = -1;
  int outpipe = -1;
  int wait = 1;
  vector <int> pids;
  
  vector <string> tokens = splitString(line_in);
  vector <vector <string> > cmds(1);
  
  for(int i = 0; i < tokens.size(); i++)
  {
    if(tokens[i] == "<")
    {
      redir_in = 1;
      in_loc = tokens[i + 1];
      tokens.erase(tokens.begin() + i);
      tokens.erase(tokens.begin() + i);
      i--;
    }
    else if(tokens[i] == ">")
    {
      redir_out = 1;
      out_loc = tokens[i + 1];
      tokens.erase(tokens.begin() + i);
      tokens.erase(tokens.begin() + i);
      i--;
    }
    else if(tokens[i] == "|")
    {
      num_pipes++;
      cmds.resize(cmds.size() + 1);
    }
    else if(tokens[i] == "&")
    {
      if(tokens.back() == "&")
      {
        wait = 0;
      }
    }
    else
    {
      cmds[num_pipes].push_back(tokens[i]);
    }
  }
  
  char **args = getArgs(cmds[0]);
  
  // cd and exit are parent functions
  if(strcmp(args[0], "cd") == 0)
  {
    if(args[1] == NULL)
    {
      chdir(home_path);
    }
    else
    {
      char *direc = args[1];
      if(chdir(direc) == -1)
      {
        if(errno == EACCES)
        {
          write(STDOUT_FILENO, "Permission denied.\n", 19);
        }
        else if(errno == ENOTDIR)
        {
          write(STDOUT_FILENO, direc, strlen(direc));
          write(STDOUT_FILENO, " not a directory!\n", 18);
        }
        else
        {
          write(STDOUT_FILENO, "Error changing directory.\n", 26);
        }
      }
    }
    getcwd(curWD, PATH_MAX);
    return;
  } 
  else if(strcmp(args[0], "exit") == 0)
  {
    alive = 0;
    return;
  }
  //fork then execute
  else 
  {
    //loop for each cmd that needs to run (1 more than pipes)
    for (int i = 0; i < num_pipes + 1; i++)
    {
      int pip[2]; 
      args = getArgs(cmds[i]); //get next set of cmds
      
      //new pipe if not the last set of cmds
      if((num_pipes > 0) & (i != num_pipes))
      {
        pipe(pip);
        inpipe = pip[0];
        outpipe = pip[1];
      }
      
      pid_t pid = fork();      
      //fork failed
      if(pid < 0)
      {
        write(STDOUT_FILENO, "Error!\n", 7);
      }
      //child process
      else if (pid == 0)
      {
        //redirect input
        if(redir_in == 1)
        {
          int fd = open(in_loc.c_str(), O_RDONLY);
          dup2(fd, STDIN_FILENO);
          close(fd);
        }
        
        //redirect output
        if(redir_out == 1)
        {
          int fd = open(out_loc.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }
              
        //redirect input from previnpipe
        if(previnpipe != -1)
        {
          dup2(previnpipe, STDIN_FILENO);
        }
        
        //redirect output to outpipe
        if(outpipe != -1)
        {
          dup2(outpipe, STDOUT_FILENO);
        }
        
        if(strcmp(args[0], "ls") == 0) 
        {
          char *dir_name;
          if(args[1] == NULL)
          {
            dir_name = curWD;
          }
          else
          {
            dir_name = args[1];
          }
          
          DIR *dir = opendir(dir_name);
          
          if(dir == NULL)
          {
            if(errno == EACCES)
              {
                write(STDOUT_FILENO, "Permission denied.\n", 19);
              }
              else if(errno == ENOTDIR)
              {
                write(STDOUT_FILENO, dir_name, strlen(dir_name));
                write(STDOUT_FILENO, " not a directory!\n", 18);
              }
              else
              {
                write(STDOUT_FILENO, "Error listing directory.\n", 25);
              }
          }
          else
          {
            struct dirent *dp;
            while((dp = readdir(dir)) != NULL)
            {
              printPermissions(dir_name, dp->d_name);
              write(STDOUT_FILENO, dp->d_name, strlen(dp->d_name));
              write(STDOUT_FILENO, "\n", 1);
            }
          }
          closedir(dir);
        }
        else if(strcmp(args[0], "pwd") == 0)
        {
          write(STDOUT_FILENO, curWD, strlen(curWD));
          write(STDOUT_FILENO, "\n", 1);
        }
        else if(strcmp(args[0], "history") == 0)
        {
          char entry;
          for(int i = 0; i < history.size(); i++)
          {
            entry = '0' + i;
            write(STDOUT_FILENO, &entry, 1);
            write(STDOUT_FILENO, " ", 1);
            write(STDOUT_FILENO, history[i].c_str(), history[i].length());
            write(STDOUT_FILENO, "\n", 1);
          }
        }
        else 
        {
          if(execvp(args[0], args) == -1)
          {
            write(STDOUT_FILENO, "Failed to execute ",18);
            write(STDOUT_FILENO, args[0], strlen(args[0]));
            write(STDOUT_FILENO, "\n", 1);
          }
          exit(1);
        }
        exit(0);
      }
      //parent process
      else
      {
        pids.push_back(pid);
        
        if(previnpipe != -1)
        {
          close(previnpipe);
        }
        
        if(outpipe != -1)
        {
          close(outpipe);
        }
        
        previnpipe = inpipe;
      }
    }
    for(int i = 0; i < pids.size(); i++)
    {  
      if(wait == 1)
      {
        int status = 0;
        waitpid(pids[i], &status, 0);
      }
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
        temp = temp.substr(slash, temp.length());
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
        write(STDOUT_FILENO, "\n", 1);
        addToHistory(line_in);
        parseInput(line_in);
        line_in = "";
        hist_index = 0;
        prPrompt = 1;
        break;
      //backspace pressed
      case 0x7F:
        if(line_in.empty())
        {
          write(STDOUT_FILENO, "\a", 1);
        }
        else
        {
          write(STDOUT_FILENO, "\b \b", 3);
          line_in.erase(line_in.length() - 1);
        }
        break;
      //control sequence pressed
      case 0x1B:      
        read(STDIN_FILENO, &char_in, 1);
        if(char_in != 0x5B)
        {
          break;
        }
        read(STDIN_FILENO, &char_in, 1);
        //up arrow
        if(char_in == 0x41)
        {
          string temp;          
          if(history.empty() || hist_index + 1 > history.size())
          {
            write(STDOUT_FILENO, "\a", 1);
            break;
          }
          hist_index++;
          temp = history[history.size() - hist_index];
          for(int i = 0; i < line_in.size(); i++)
          {
            write(STDOUT_FILENO, "\b \b", 3);
          }
          write(STDOUT_FILENO, temp.c_str(),temp.length());
          line_in = temp;
          break;
        }
        //down arrow
        else if(char_in == 0x42)
        {
          string temp;
          if(hist_index == 0)
          {
            write(STDOUT_FILENO, "\a", 1);
            break;
          }
          hist_index--;
          if(hist_index == 0)
          {
            temp = "";
          }
          else
          {
            temp = history[history.size() - hist_index];
          }
          for(int i = 0; i < line_in.size(); i++)
          {
            write(STDOUT_FILENO, "\b \b", 3);
          }
          write(STDOUT_FILENO, temp.c_str(),temp.length());
          line_in = temp;
          break;
        }
        //other control sequence
        else
        {
          break;
        }
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