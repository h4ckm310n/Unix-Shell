#include "project.h"
vector<string> cmd_history;
vector<pid_t> recent_pid;
map<int, Job*> jobs;
int max_jn = 0;
Parse parse;
Execution exec;


int main()
{
	string line;
	char curr_path[100];
	exec.start("clear");
	char hostname[30];
	gethostname(hostname, 30);
	
	//ignore signals
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	int shell_pid = getpid();
	while (1)
	{
		//current directory
		getcwd(curr_path, sizeof(curr_path));
		cout << "[" << hostname << ": " << curr_path << "]$ ";
		getline(cin, line);
		if (line == "")
			continue;
		exec.start(line);
		//history
		cmd_history.push_back(line);
	}
}
