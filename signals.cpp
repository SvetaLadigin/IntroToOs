#include <iostream>
#include <csignal>
#include "signals.h"
#include "Commands.h"
#include <cstring>

using namespace std;
extern SmallShell& smash;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z" << endl;
    pid_t pid = smash.getCurrPid();

    if(pid <= 0) return; //not succeed

    //sending signal
    if(killpg(pid,SIGSTOP)==-1){
        perror("smash error: stop failed");
        return;
    }

    char tmp_cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(tmp_cmd, smash.getCurrCmd().c_str());
    JobsList::JobEntry* job = smash.getJobsListRef().getJobByPid(pid);

    if(job != nullptr){  //already in the list
        cerr << "smash: process " << pid << " was in backgroung" << endl;
        return;
    }
    else{ //not on jobs list
        smash.getJobsListRef().addJob(tmp_cmd, pid, false);
    }

    cout << "smash: process " << pid << " was stopped" << endl;
}


void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

