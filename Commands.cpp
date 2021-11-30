#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>
#include <time.h>

const std::string WHITESPACE = " \n\r\t\f\v";

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  SmallShell &smash = SmallShell::getInstance();

  if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(this, cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, getPrevPwd());
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line, nullptr, this);    ///exchange nullptr with jobs_list ptr
  }
  else if (firstWord.compare("jobs") == 0){
      return new JobsCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
    Command* cmd;
//    SmallShell &smash = SmallShell::getInstance();
    if(_isBackgroundComamnd(cmd_line)){ ///if the input ends with '&' remove it and mark the command as background command
        char new_cmd[COMMAND_ARGS_MAX_LENGTH] = {0};
        strcpy(new_cmd, cmd_line);
        _removeBackgroundSign(new_cmd);
        cmd = CreateCommand(new_cmd);
        cmd->setAsBackground();
//        smash.getJobsListRef().addJob(cmd_line, this->smashPid, true);

    } else {  ///foreground command
        cmd = CreateCommand(cmd_line);
    }

    if(cmd){    ///can CreateCommand fail?
        cmd->prepare();
        cmd->execute();
        cmd->cleanup();
        delete cmd;
    } else {    ///CreateCommand failure error message
    }
}

JobsList &SmallShell::getJobsListRef() {
        return this->jobs;
    }

JobsList SmallShell::getJobList() { //TODO do i really need it?
    return jobs;
}

pid_t SmallShell::getCurrPid() {
    return currPid;
}

pid_t SmallShell::getSmashPid() {
    return smashPid;
}


Command::Command(const char* cmd_line) : cmd_line(cmd_line) {};

void Command::prepare() {
    _parseCommandLine(this->cmd_line, this->args);
}

void Command::cleanup() {
    int i = 0;
    while(args[i] != NULL) {
        free(args[i]);
        i++;
    }
}


BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {};

ChangePromptCommand::ChangePromptCommand(SmallShell* smash, const char* cmd_line) : BuiltInCommand(cmd_line), smash(smash) {};

void ChangePromptCommand::execute() {
    if(this->getArgs()[1] != NULL){
        smash->setPrompt(string(getArgs()[1]));
    }
    else
        smash->setPrompt("smash");
}


ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}


GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void GetCurrDirCommand::execute() {
    char buf[PATH_MAX];
    std::cout << getcwd(buf, sizeof(buf)) << std::endl;
}


ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string* prevPwd) : BuiltInCommand(cmd_line), prevPwd(prevPwd) {};

void ChangeDirCommand::execute() {
    if(this->getArgs()[1] == NULL)  ///ignore "cd" without any arguments
        return;

    if(this->getArgs()[2] != NULL) { ///reject "cd" with more than 1 argument
        std::cerr << "smash error: cd: too many arguments" << endl;
        return;
    }

    char buf[PATH_MAX];
    string currentDir = getcwd(buf, sizeof(buf));

    int val = 2;

    if(string(this->getArgs()[1]) == "-") {///"cd -"
        if(*prevPwd == "")
            std::cerr << "smash error: cd: OLDPWD not set" << endl;
        else {
            val = chdir(prevPwd->c_str());
        }
    } else {///"cd new_dir"
        val = chdir(this->getArgs()[1]);
    }

    if(val == -1)
        perror("smash error: chdir failed");
    else if (val == 0)  ///after successful dir change store old dir in prevPwd
        *prevPwd = currentDir;
}


QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs, SmallShell* smash) : BuiltInCommand(cmd_line), jobs(jobs), smash(smash) {};

void QuitCommand::execute() {
    if(((this->getArgs()[1]) != NULL) && (string(this->getArgs()[1]) == "kill")) {
    /// kill (by sending SIGKILL signal) all of its unfinished and stopped jobs
    /// print (before exiting) the number of processes/jobs that were killed, their PIDs and command-lines
    }

    smash->setToQuit();
}


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {};

void ExternalCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid = fork();

    if(pid == 0) {
        setpgrp();

        char* bash_path = (char*)"/bin/bash";
        char* bash_flags = (char*)"-c";
        char cmd[COMMAND_ARGS_MAX_LENGTH] = {0};
        strcpy(cmd, getCmdLine());

        char* args[] = {bash_path, bash_flags, cmd, NULL};
        if(execv(args[0], args) == -1)
            perror("smash error: exec failed");
    } else {
        if(this->isBackground()){
            char temp[COMMAND_ARGS_MAX_LENGTH];
            strcpy(temp, this->getCmdLine());
            smash.getJobsListRef().addJob(this->getCmdLine(), pid, true);
        }

       if(isForeground())
            if(waitpid(pid,nullptr,0) == -1)
                perror("smash error: waitpid failed");
    }
}

void JobsList::addJob(string cmd, pid_t pid, bool active_status) {
    //getting the instance
    SmallShell &smash = SmallShell::getInstance();
    int curr_jobid = smash.getJobsListRef().getMaxId(); //getting the max id to add to the job list
    auto* new_job = new JobEntry(cmd, pid,curr_jobid + 1 ,active_status); //new job entry
    smash.getJobsListRef().setMaxId(curr_jobid+1); //setting to list with new max
    //adding to list
    auto it= jobs_list->begin();
    if(*it == nullptr){
        jobs_list->insert(it,new_job);
    }
    else{
        jobs_list->push_back(new_job);
    }

}



void JobsList::printJobsList() {

    removeFinishedJobs(); //remove all finished jobs before printing
    if(!this->jobs_list->empty()){
        //iterating over the list
        auto it = jobs_list->begin();
        while(it != jobs_list->end())
        {
            //check active status
            if((*it)->getActiveStatus()){
                std::cout << "[" << (*it)->getJobId() << "] " << (*it)->getCmd() << " : " << (*it)->getPID() << " " << difftime(time(nullptr), (*it)->getTime()) << endl;
            }
            else{
                    std::cout << "[" << (*it)->getJobId() << "] " << (*it)->getCmd() << " : " << (*it)->getPID() << " " << difftime(time(nullptr),(*it)->getTime()) <<" (stopped)" << endl;
                }
            ++it;
        }
    }
}

void JobsList::killAllJobs() {
        removeFinishedJobs(); //TODO do we really need it?
        if(!jobs_list->empty())
        {
            auto it=jobs_list->begin();
            while(it != jobs_list->end())
            {
                if(killpg((*it)->getPID(), SIGKILL)==-1)
                {
                    perror("smash error: kill failed");
                    return;
                }

                ++it;
            }
        }
}

void JobsList::removeFinishedJobs() {
    if(!jobs_list->empty())
    {
        auto it = jobs_list->begin();

        while((it != jobs_list->end()) && (!jobs_list->empty()))
        {
            pid_t pid = (*it)->getPID();
            int ret = waitpid(pid, nullptr, WNOHANG);
            if(ret == (*it)->getPID()) //check if job finished
            {
                auto temp = it;
                it++;
                jobs_list->erase(temp);

            }
            else{
                it++;
            }
        }
    }
    if(jobs_list->empty())
        setMaxId(0);
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    if(!jobs_list->empty())
    {
        auto it = jobs_list->begin();
        while(it != jobs_list->end())
        {
            if((*it)->getJobId() == jobId)
                return *it;

            ++it;
        }
    }

    return nullptr;
}

void JobsList::removeJobById(int jobId) {
        if(!jobs_list->empty())
        {
            auto it= jobs_list->begin();
            while(it != jobs_list->end())
            {
                if((*it)->getJobId() == jobId)
                {
                    jobs_list->erase(it);
                    return;
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            setMaxId(0);
        }
    }

JobsList::JobEntry* JobsList::getJobByPid(pid_t job_pid){
    if(!jobs_list->empty())
    {
        auto it = jobs_list->begin();
        while(it != jobs_list->end())
        {
            if((*it)->getPID() == job_pid)
                return *it;

            ++it;
        }
    }

    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    
        if(!jobs_list->empty())
        {
            return jobs_list->back();
        }

        return nullptr;
    }


JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry* last_stopped = nullptr;
    int last_jobid = 0;

    if(!jobs_list->empty())
    {
        auto it=jobs_list->begin();
        while(it != jobs_list->end())
        {
            if(((*it)->getJobId() > last_jobid) && ((*it)->getActiveStatus()))
            {
                last_stopped = *it;
                last_jobid = (*it)->getJobId();
            }

            ++it;
        }
    }

    return last_stopped; //TODO POTENTIAL MEMORY LEAK
}

int JobsList::getMaxId() {
    return this->max_id;
}


void JobsList::setMaxId(int new_id) {
this->max_id = new_id;
}

JobsList::~JobsList(){

    this->jobs_list->clear();
    delete jobs_list;

}

pid_t JobsList::JobEntry::getPID() {
    return this->job_pid;
}

int JobsList::JobEntry::getJobId() {
    return this->job_id;
}

bool JobsList::JobEntry::getActiveStatus() {
    return this->active_status;
}

string JobsList::JobEntry::getCmd() {
    return this->command;
}

time_t JobsList::JobEntry::getTime() {
    return this->start_time;
}

void JobsList::JobEntry::resetTheTime() {
        this->start_time=time(nullptr);
        if(start_time==-1){
            perror("smash error: time failed");
        }

}

void JobsList::JobEntry::setActiveStatus(bool status) {
this->active_status = status;
}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute(){
    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().printJobsList();
    }

bool CheckInput(const char *cmd_line){
    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().removeFinishedJobs(); //delete finished jobs
//    auto *arguments = this->getArgs(); //TODO implement check input

//
//    if( != 3){
//        std::cout << "smash error: kill: invalid arguments" << endl;
//        return;
//    }
//
//    if(!is_digits(this->getArgs()[1]) || !is_digits(this->getArgs()[2]))
//    {
//        std::cout << "smash error: kill: invalid arguments" << endl;
//        return;
//    }
//
//    int orgSignum = stoi(this->getArgs()[1]);
//    int signum = orgSignum*(-1);
//
//    if((signum <= 0) && (signum >= 32)){
//        std::cout << "smash error: kill: invalid arguments" << endl;
//        return;
//    }
return false;
}
KillCommand::KillCommand(const char *cmd_line):BuiltInCommand(cmd_line){}

void KillCommand::execute(){

    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().removeFinishedJobs(); //delete finished jobs
    int signum_with_minus = stoi(this->getArgs()[1]);
    int signum = signum_with_minus*(-1);
    if(smash.getJobsListRef().getJobById(stoi(this->getArgs()[2])) == nullptr)
    {
        std::cout << "smash error: kill: job-id " << stoi(this->getArgs()[2]) << " does not exist" << endl;
        return;
    }
    pid_t pid = smash.getJobsListRef().getJobById(stoi(this->getArgs()[2]))->getPID();
    if(killpg(pid, signum) == -1){
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << signum << " was sent to pid " << pid << endl;
    smash.getJobsListRef().removeFinishedJobs(); //clean list

}


