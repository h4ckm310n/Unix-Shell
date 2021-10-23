#include "project.h"
#include <fcntl.h>


int Execution::start(string line)
{
	check_bg();
	line = strip_space(line);
	vector<string> commands = parse.multi(line);
	vector<cmdvec> construct_commands;
	CommonCMD cmd;
	LogicCMD lcmd;
	RedirCMD rcmd;
	PipeCMD *pcmd;
	for (int ic=0; ic<commands.size(); ++ic)
	{
		lcmd = parse.logic(commands[ic]);
		if (lcmd.symbol != -1)
		{
			this->logic(lcmd);
			continue;
		}
		rcmd = parse.redir(commands[ic]);
		if (rcmd.mode != -1)
		{
			this->redir(rcmd);
			continue;
		}
		pcmd = parse.pipe(commands[ic]);
		if (pcmd->right != NULL)
		{
			this->_pipe(pcmd);
			continue;
		}
		cmd = parse.single(commands[ic]);
		if (cmd.cmd.size() != 0)
		{
			this->single(cmd);
		}
	}
	return 1;
}


int Execution::cont(string mode, int num)
{
	int child = jobs.at(num)->pid;
	int bg = jobs.at(num)->bg;
	kill(child, SIGCONT);
	jobs.at(num)->status = 1;
	int s;
	if (mode == "continue")
	{
		if (bg)
			s = 0;
		else
			s = this->_wait(child, num);
	}
	else if (mode == "bg")
	{
		jobs.at(num)->bg = 1;
		s = 0;
	}
	else if (mode == "fg")
	{
		jobs.at(num)->bg = 0;
		s = this->_wait(child, num);
	}
	return s;
}


int Execution::_wait(int pid, int num)
{
	int status;
	waitpid(pid, &status, WUNTRACED);
	if (WIFSIGNALED(status))
		jobs.at(num)->status = 3;
	else if (WIFEXITED(status))
		jobs.at(num)->status = 0;
	else if (WSTOPSIG(status))
	{
		jobs.at(num)->status = 2;
		printf("[%d] Suspended\n", num);
	}
	int s = WEXITSTATUS(status);
	return s;
}


int Execution::exec(char **cmd)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	if (strcmp(cmd[0], "history") == 0)
	{
		for (int i=0; i<cmd_history.size(); ++i)
			cout << i+1 << " " << cmd_history[i] << endl;
		exit(0);
	}

	if (strcmp(cmd[0], "pid") == 0)
	{
		for (int i=recent_pid.size()-1; i>=0; --i)
		{
			printf("[%d] %d\n", i+1, recent_pid[i]);
		}
		exit(0);
	}

	int append=0;
	glob_t globbuf;
	for (int i=0;cmd[i]!=NULL;i++){
		if (strstr(cmd[i],"*")) {
			if(append==0){
				globbuf.gl_offs = i; 
				glob(cmd[i], GLOB_DOOFFS, NULL, &globbuf); 
			}
			else {glob(cmd[i], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf); }
			append=1;
		}
	}
	if (append==1){
		for (int j=0;!strstr(cmd[j],"*");j++){
			globbuf.gl_pathv[j] = cmd[j];
		}
		execvp(cmd[0], &globbuf.gl_pathv[0]);
	}

	execvp(cmd[0], cmd);
	//error
	cerr << "-OS Shell: " << cmd[0] << ": command not found\n";
	exit(1);
}


int Execution::single(CommonCMD c)
{
	if (c.cmd[0] == "exit")
	{
		while (wait(NULL) > 0);
		exit(0);
	}

	if (c.cmd[0] == "go")
	{
		if (c.cmd.size() == 1)
		{
			cout << "Error: go command without argument.\n";
			return -1;
		}
		chdir(c.cmd[1].c_str());
		return 0;
	}

	if (c.cmd[0] == "continue" || c.cmd[0] == "bg" || c.cmd[0] == "fg")
	{
		int s = this->cont(c.cmd[0], atoi(c.cmd[1].c_str()));
		return s;
	}

	if (c.cmd[0] == "jobs")
	{
		string status[] = {"Done", "Running", "Suspended", "Terminated"};
		for (map<int, Job*>::iterator i = jobs.begin(); i != jobs.end(); i++)
			printf("[%d] %d %s %s\n", i->second->num, i->second->pid, i->second->cmd.c_str(), status[i->second->status].c_str());
		return 0;
	}

	if (c.cmd[0] == "status")
	{
		printf("%d\n", jobs.rbegin()->second->status);
		return 0;
	}

	//convert to c-string
	Command command(c.cmd);

	pid_t pid = fork();
	max_jn++;
	int jobnum = max_jn;
	if (pid == 0)
	{
		// child process for exec
		this->exec(command.cmd);
	}
	else
	{
		//add pid history
		recent_pid_add(pid);
		jobs[jobnum] = new Job(c.cmd, jobnum, 1, pid, c.bg);
		int s;
		if (!c.bg)
			s = this->_wait(pid, jobnum);
		else
		{
			printf("[%d] %d\n", jobnum, pid);
			s = 0;
		}
		return s;
	}
	return 1;
}


int Execution::logic(LogicCMD l)
{
	CommonCMD left = parse.single(l.left);
	CommonCMD right = parse.single(l.right);
	int exec_res = this->single(left);

	if ((l.symbol == 2 && exec_res != 0) || (l.symbol == 1 && exec_res == 0))
		this->single(right);
	return 0;
}


int Execution::_pipe(PipeCMD *p)
{
	int pipe_n = 0;
	PipeCMD *p2 = p;
	while (p2->right != NULL)
	{
		pipe_n++;
		p2 = p2->right;
	}
	int **fds = new int *[pipe_n];
	int i;
	for (i=0; i<pipe_n; ++i)
		fds[i] = new int[2];
	vector<size_t> pids;
	i = 0;
	p2 = p;
	pipe(fds[0]);
	while (1)
	{
		Command cl(parse.single(p2->left).cmd);
		pid_t pidl = fork();
		if (pidl == 0)
		{
			int j = 0;
			while (j < i-1)
			{
				close(fds[j][0]);
				close(fds[j][1]);
				++j;
			}
			close(fds[i][0]);
			if (i > 0)
			{
				close(fds[i-1][1]);
				dup2(fds[i-1][0], STDIN_FILENO);
				close(fds[i-1][0]);
			}
			dup2(fds[i][1], STDOUT_FILENO);
			close(fds[i][1]);
			this->exec(cl.cmd);
		}
		else
		{
			recent_pid_add(pidl);
			pids.push_back(pidl);
		}
			
		
		if (p2->right->right == NULL)
		{
			Command cr(parse.single(p2->right->left).cmd);
			pid_t pidr = fork();
			if (pidr == 0)
			{
				int j = 0;
				while (j < i)
				{
					close(fds[j][0]);
					close(fds[j][1]);
					++j;
				}
				close(fds[i][1]);
				dup2(fds[i][0], STDIN_FILENO);
				close(fds[i][0]);
				this->exec(cr.cmd);
			}
			else
			{
				recent_pid_add(pidr);
				pids.push_back(pidr);
			}
			break;
		}
		else
		{
			p2 = p2->right;
			pipe(fds[i+1]);
		}
		++i;
	}
	for (i=0; i<pipe_n; ++i)
	{
		close(fds[i][0]);
		close(fds[i][1]);
	}
	for (i=0; i<pids.size(); ++i)
	{
		waitpid(pids[i], NULL, 0);
	}
	return 1;
}


int Execution::redir(RedirCMD r)
{
	int sfd = dup(r.mode);
	int rwf;

	//output
	if (r.mode == 1)
		rwf = open(r.file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
	//input
	else
		rwf = open(r.file.c_str(), O_RDONLY);
	dup2(rwf, r.mode);
	PipeCMD *p = parse.pipe(r.cmd);
	if (p->right != NULL)
	{
		this->_pipe(p);
	}
	else
	{
		this->single(parse.single(p->left));
	}
	dup2(sfd, r.mode);
	close(rwf);
	return 0;
}

