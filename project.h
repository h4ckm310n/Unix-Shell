#ifndef _PROJECT_H_

#define _PROJECT_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <signal.h>

using namespace std;

typedef vector<string> cmdvec;


class RedirCMD
{
public:
	int mode;
	string cmd;
	string file;
};

class PipeCMD
{
public:
	string left;
	PipeCMD *right;
};

class LogicCMD
{
public:
	string left;
	string right;
	int symbol;
};

class CommonCMD
{
public:
	int bg;
	cmdvec cmd;
};


class Parse
{
public:
	CommonCMD single(string cmd);
	vector<string> multi(string line);
	LogicCMD logic(string cmd);
	RedirCMD redir(string cmd);
	PipeCMD *pipe(string cmd);
};


class Execution
{
public:
	int start(string line);
	int single(CommonCMD c);
	int logic(LogicCMD l);
	int _pipe(PipeCMD *p);
	int redir(RedirCMD r);
	int exec(char **cmd);
	int _wait(int pid, int num);
	int cont(string mode, int num);
private:
	int rwfile;
	int pipefd[2];
};

extern Parse parse;
extern Execution exec;


extern vector<string> cmd_history;
extern vector<pid_t> recent_pid;


class Command
{
public:
	Command(cmdvec v);
	char **cmd;
	int size;
};


class Job
{
public:
	Job(cmdvec cmd, int num, int status, int pid, int bg);
	string cmd;
	int pid;
	int num;
	int status;
	int bg;
};
extern map<int, Job*> jobs;
extern int max_jn;

void recent_pid_add(pid_t pid);
string strip_space(string str);
void check_bg();


#endif
