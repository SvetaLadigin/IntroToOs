#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
 #include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {

    SmallShell& smash = SmallShell::getInstance();

    std::cout << "smash: got ctrl-Z" << endl;
    if(smash.getCurrPid()!= -1) {
        if(killpg(smash.getCurrPid(), SIGSTOP) == -1) {
            perror("smash error: kill failed");
            return;
        }

        std::cout << "smash: process " << smash.getCurrPid() << " was stopped" << endl;

        if(JobsList::JobEntry* jobs = smash.getJobsListRef().getJobByPid(smash.getCurrPid())) {
            jobs->setActiveStatus(false);
            jobs->resetTheTime();
        } else {
            smash.getJobsListRef().addJob(smash.getCurrCmd(), smash.getCurrPid(), false);
        }
        smash.setCurrPid(-1);
        smash.setCurrCmd("");
    }

}

void ctrlCHandler(int sig_num) {

    SmallShell& smash = SmallShell::getInstance();

    std::cout << "smash: got ctrl-C" << endl;
    if(smash.getCurrPid()!= -1) {
        if(killpg(smash.getCurrPid(), SIGKILL) ==-1) {
            perror("smash error: kill failed");
            return;
        }

        if(smash.getJobsListRef().getJobByPid(smash.getCurrPid()))
            smash.getJobsListRef().removeJobById(smash.getJobsListRef().getJobByPid(smash.getCurrPid())->getJobId());
        std::cout << "smash: process " << smash.getCurrPid() << " was killed" << endl;
        smash.setCurrPid(-1);
        smash.setCurrCmd("");
    }

}

void alarmHandler(int sig_num) {}
