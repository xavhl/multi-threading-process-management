#include <iostream>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <list>
#include <string>
#include <queue>
using namespace std;

class Process {
public:
	pid_t pid;
	char* name;
	string status;

	Process(pid_t pid, char* name, string status) {
		this->pid = pid;
		this->name = name;
		this->status = status;
	}
};

int main(int argc, const char* argv[]) {
	int argIndex = 0, status;
	int count = 0;//number of running process
	pid_t pid = 0, ppid = getpid(), wnohang = 0;

	string argStr, token;
	size_t cur_token = 0, next_token;
	char* argList[129], * cmd;

	list<Process*> PCB;
	list<Process*>::iterator itr;
	queue<Process*> stoppedP;
	bool checkStopped = false;//check and restart, if any process in stopped state


	while (1) {
		if (pid != 0)//if not first time/ loop, will check previous child processes
			wnohang = waitpid(0, &status, WNOHANG);

		if (wnohang > 0) {//if child status changed, update PCB list
			for (itr = PCB.begin(); itr != PCB.end(); itr++) {
				if ((*itr)->pid == wnohang) {
					if ((*itr)->status != "terminated") {
						cout << wnohang << " completed\n";
						(*itr)->status = "terminated";

						count--;
						checkStopped = true;
					}
				}
			}
		}
		else {//no change
			cout << "BP <";
			getline(cin, argStr);

			//parse string into char* arrays
			next_token = argStr.find_first_of(" ", cur_token);
			token = argStr.substr(cur_token, next_token - cur_token);

			if (next_token != string::npos)
				cur_token = next_token + 1;

			char* arg = new char[token.size() + 1];
			strncpy(arg, token.c_str(), token.size() + 1);//convert token from str to char*
			cmd = arg;

			while (next_token != string::npos) {
				next_token = argStr.find_first_of(" ", cur_token);
				token = argStr.substr(cur_token, next_token - cur_token);

				if (next_token != string::npos)
					cur_token = next_token + 1;

				char* arg = new char[token.size() + 1];
				strncpy(arg, token.c_str(), token.size() + 1);//convert token from str to char*
				argList[argIndex++] = arg;
			}
			argList[argIndex] = NULL;

			//check commands
			if (strncmp(cmd, "bglist", 6) == 0) {
				for (itr = PCB.begin(); itr != PCB.end(); itr++) {
					cout << (*itr)->pid << ": " << (*itr)->name << "(" << (*itr)->status << ")\n";
				}
			}
			else if (strncmp(cmd, "bgkill", 6) == 0) {
				int targetid = atoi(argList[0]);
				bool inStructure = false;

				cout << targetid;

				for (itr = PCB.begin(); itr != PCB.end(); itr++) {
					if ((*itr)->pid == targetid) {
						inStructure = true;

						if ((*itr)->status == "terminated") {
							cout << " already terminated\n";
						}
						else {
							kill((*itr)->pid, SIGTERM);
							cout << " killed\n";

							if ((*itr)->status != "stopped")//stopped process doesnt count as had been decremented
								count--;

							(*itr)->status = "terminated";
							checkStopped = true;
						}
						break;
					}
				}

				if (!inStructure)
					cout << " does not exist\n";
			}
			else if (strncmp(cmd, "bgstop", 6) == 0) {
				int targetid = atoi(argList[0]);
				bool inStructure = false;

				cout << targetid;

				for (itr = PCB.begin(); itr != PCB.end(); itr++) {
					if ((*itr)->pid == targetid) {
						inStructure = true;

						if ((*itr)->status == "stopped") {
							cout << " already stopped\n";
						}
						else if ((*itr)->status == "terminated") {
							cout << " already terminated\n";
						}
						else {
							kill((*itr)->pid, SIGSTOP);
							cout << " stopped\n";
							(*itr)->status = "stopped";

							count--;
							checkStopped = true;
						}
						break;
					}
				}

				if (!inStructure)
					cout << " does not exist\n";
			}
			else if (strncmp(cmd, "bg", 2) == 0) {
				pid = fork();

				if (pid > 0) {//parent
					if (count < 3) {
						PCB.push_back(new Process(pid, argList[0], "running"));
						count++;
					}
					else {//exceeds limit
						PCB.push_back(new Process(pid, argList[0], "stopped"));
						stoppedP.push(PCB.back());
						kill(pid, SIGSTOP);
					}
				}
				else if (pid == 0) {//child
					if (execvp(argList[0], argList) < 0)
						cout << getpid() << "execvp() failed...\n";
				}
				else {
					cout << "fork() failed...\n";
					exit(EXIT_FAILURE);
				}
			}
			else if (strncmp(cmd, "exit", 4) == 0) {
				for (itr = PCB.begin(); itr != PCB.end(); itr++) {
					if ((*itr)->status != "terminated") {
						cout << (*itr)->pid << " killed\n";
					}
				}

				kill(0, SIGTERM);
				break;
			}
			else {
				cout << "invalid command..." << endl;
			}

			argIndex = 0;//reset for next cmd
			cur_token = 0;
			memset(argList, 0, sizeof(argList));
		}

		if (checkStopped) {
			if (!stoppedP.empty()) {
				for (int j = 0; j < 3 - count && !stoppedP.empty(); j++) {
					kill(stoppedP.front()->pid, SIGCONT);

					stoppedP.front()->status = "running";
					cout << stoppedP.front()->pid << " automatically restarted\n";

					stoppedP.pop();
					count++;
				}
			}
			checkStopped = false;
		}
	}

	return 0;
}