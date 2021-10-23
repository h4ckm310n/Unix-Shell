#include "project.h"

CommonCMD Parse::single(string cmd)
{
	CommonCMD command;
	string temp = "";
	int quote_str = 0;
	int bg = 0;
	for (int i=0; i<cmd.size(); ++i)
	{
		if (cmd[i] == '"')
		{
			if (!quote_str)
			{
				quote_str = 1;
			}
			else
			{
				command.cmd.push_back(temp);
				temp = "";
				quote_str = 0;
			}
			continue;
		}
		if (quote_str)
		{
			temp = temp + cmd[i];
			continue;
		}
		if (cmd[i] == '&' && i == cmd.size()-1)
		{
			bg = 1;
			continue;
		}

		if (cmd[i] != ' ')
		{
			temp = temp + cmd[i];
			if (i == cmd.size() - 1)
				command.cmd.push_back(temp);
			continue;
		}
		command.cmd.push_back(temp);
		temp = "";
	}
	command.bg = bg;
	return command;
}


vector<string> Parse::multi(string line)
{
	//seperated by ';' in a line

	vector<string> cmds;
	string temp = "";
	for (int i=0; i<line.size(); ++i)
	{
		if (line[i] != ';')
		{
			temp = temp + line[i];
			if (i == line.size() - 1)
				cmds.push_back(strip_space(temp));
			continue;
		}
		cmds.push_back(strip_space(temp));
		temp = "";
	}
	return cmds;
}


PipeCMD *Parse::pipe(string cmd)
{
	PipeCMD *p = new PipeCMD;
	size_t find_pos = cmd.find('|');
	if (find_pos == string::npos)
	{
		p->right = NULL;
		p->left = cmd;
	}
	else
	{
		p->left = strip_space(cmd.substr(0, find_pos-1));
		p->right = this->pipe(strip_space(cmd.substr(find_pos+1)));
	}
	return p;
}


RedirCMD Parse::redir(string cmd)
{
	RedirCMD r;

	size_t find_pos = cmd.find('<');
	if (find_pos == string::npos)
		find_pos = cmd.find('>');
	if (find_pos == string::npos)
	{
		r.mode = -1;
		r.cmd = cmd;
	}
	else
	{
		r.mode = cmd[find_pos] == '<' ? 0 : 1;
		r.file = strip_space(cmd.substr(find_pos+1));
		r.cmd = strip_space(cmd.substr(0, find_pos-1));
	}
	return r;
}

LogicCMD Parse::logic(string cmd)
{
	LogicCMD l;

	size_t find_pos = cmd.find("||");
	if (find_pos == string::npos)
	{
		find_pos = cmd.find("&&");
		if (find_pos == string::npos)
			l.symbol = -1;
		else
			l.symbol = 1;
	}
	else
		l.symbol = 2;

	if (l.symbol != -1)
	{
		l.left = strip_space(cmd.substr(0, find_pos-1));
		l.right = strip_space(cmd.substr(find_pos+2));
	}
	return l;
}
