#include "project.h"


void recent_pid_add(pid_t pid)
{
	recent_pid.push_back(pid);

	//if more than 5, erase
	while (recent_pid.size() > 5)
		recent_pid.erase(recent_pid.begin());
}


string strip_space(string str)
{
	str.erase(0, str.find_first_not_of("\r\t\n "));
	str.erase(str.find_last_not_of("\r\t\n ") + 1);
	return str;
}


void check_bg()
{
	for (map<int, Job*>::iterator i = jobs.begin(); i != jobs.end(); i++)
	{
		if (i->second->bg)
		{
			int status;
			int p_exit = waitpid(i->second->pid, &status, WNOHANG);

			//exit
			if (p_exit == i->second->pid)
			{
				if (WIFSIGNALED(status))
					i->second->status = 3;
				else if (WIFEXITED(status))
					i->second->status = 0;
			}
		}
	}
}


Command::Command(cmdvec v)
{
	this->cmd = new char *[v.size()+1];
	for (int i=0; i<v.size(); ++i)
	{
		this->cmd[i] = new char[v[i].size()];
		strcpy(this->cmd[i], v[i].c_str());
	}
	this->cmd[v.size()] = NULL;
	this->size = v.size();
}


Job::Job(cmdvec cmd, int num, int status, int pid, int bg)
{
	this->cmd = "";
	for (int i=0; i<cmd.size(); ++i)
	{
		this->cmd = this->cmd + cmd[i];
		this->cmd = this->cmd + " ";
	}
	this->cmd = strip_space(this->cmd);
	this->num = num;
	this->status = status;
	this->pid = pid;
	this->bg = bg;
}
