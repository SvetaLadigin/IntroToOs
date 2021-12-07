#include <unistd.h>
#include <string.h>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>
#include <time.h>
#include <fcntl.h>

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


SmallShell::SmallShell() {}

SmallShell::~SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if((string(cmd_line).find('>')!=string::npos)){
    return new RedirectionCommand(cmd_line);
  }
  else if((string(cmd_line).find('|')!=string::npos)){
    return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
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
    return new QuitCommand(cmd_line, nullptr, this);
  }
  else if (firstWord.compare("jobs") == 0){
      return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0){
      return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0){
      return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("bg") == 0){
      return new BackgroundCommand(cmd_line);
  }
  else if (firstWord.compare("head") == 0){
      return new HeadCommand(cmd_line);
  }


  else {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {

    Command* cmd;

    if(_isBackgroundComamnd(cmd_line)){ ///if the input ends with '&' remove it and mark the command as background command
        char new_cmd[COMMAND_ARGS_MAX_LENGTH] = {0};
        strcpy(new_cmd, cmd_line);
        _removeBackgroundSign(new_cmd);
        cmd = CreateCommand(new_cmd);
        cmd->setAsBackground();
        cmd->setOgCmdLine(cmd_line);

    } else {  ///foreground command
        cmd = CreateCommand(cmd_line);
        cmd->setOgCmdLine(cmd_line);
    }

    if(cmd) {
        cmd->prepare();
        cmd->execute();
        cmd->cleanup();
        delete cmd;
    }
}

JobsList &SmallShell::getJobsListRef() {
        return this->jobs;
    }

JobsList SmallShell::getJobList() {
    return jobs;
}

pid_t SmallShell::getCurrPid() {
    return currPid;
}

pid_t SmallShell::getSmashPid() {
    return smashPid;
}

void SmallShell::setCurrPid(pid_t pid) {
    this->currPid=pid;
}

void SmallShell::setCurrCmd(string cmd) {
    this->currCmd=std::move(cmd);
}


Command::Command(const char* cmd_line) : cmd_line(cmd_line){};

void Command::prepare() {
    argc =_parseCommandLine(this->cmd_line, this->args);
}

void Command::cleanup() {
    int i = 0;
    while(args[i] != NULL) {
        free(args[i]);
        i++;
    }
}

int Command::getArgc() {
    return this->argc;
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

    if(val == -1) {
        perror("smash error: chdir failed");
        return;
    } else if (val == 0)  ///after successful dir change store old dir in prevPwd
        *prevPwd = currentDir;
}


QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs, SmallShell* smash) : BuiltInCommand(cmd_line), jobs(jobs), smash(smash) {};

void QuitCommand::execute() {
    if(((this->getArgs()[1]) != nullptr) && (string(this->getArgs()[1]) == "kill")) {
    int num_of_jobs = smash->getJobsListRef().getNumOfJobs();
    cout<<"smash: sending SIGKILL signal to "<< num_of_jobs<<" jobs:"<<endl;
    smash->getJobsListRef().killAllJobs();
    }
    else{
        smash->getJobsListRef().killAllJobNotPrint();
    }
    smash->setToQuit();
}


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {};

void ExternalCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid = fork();

    if(pid == -1) {
        perror("smash error: pid failed");
        return;
    }

    if(pid == 0) {
        if(setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }

        char* bash_path = (char*)"/bin/bash";
        char* bash_flags = (char*)"-c";
        char cmd[COMMAND_ARGS_MAX_LENGTH] = {0};
        strcpy(cmd, getCmdLine());

        char* args[] = {bash_path, bash_flags, cmd, NULL};
        if(execv(args[0], args) == -1) {
            perror("smash error: exec failed");
            exit(1);
        }
    } else {
        if(this->isBackground()){
            char temp[COMMAND_ARGS_MAX_LENGTH];
            strcpy(temp, this->getCmdLine());
            smash.getJobsListRef().addJob(this->getOgCmdLine(), pid);
        }

       if(isForeground()) {
            smash.setCurrPid(pid);
            smash.setCurrCmd(getOgCmdLine());

            if(waitpid(pid,nullptr,WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
                return;
            }

            SmallShell::getInstance().setCurrPid(-1);
            SmallShell::getInstance().setCurrCmd("");
        }
    }
}


void JobsList::addJob(string cmd, pid_t pid, bool active_status) {
    //getting the instance
    SmallShell &smash = SmallShell::getInstance();
    int curr_jobid = smash.getJobsListRef().getMaxId(); //getting the max id to add to the job list
    int new_job_id = curr_jobid+1;
    auto* new_job = new JobEntry(std::move(cmd), pid,new_job_id ,active_status); //new job entry
    new_job->setActiveStatus(active_status);
    new_job->setJobId(new_job_id);
    smash.getJobsListRef().setMaxId(new_job_id); //setting to list with new max
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
                std::cout << "[" << (*it)->getJobId() << "] " << (*it)->getCmd() << " : " << (*it)->getPID() << " " << difftime(time(nullptr), (*it)->getTime())<<" secs" << endl;
            }
            else{
                    std::cout << "[" << (*it)->getJobId() << "] " << (*it)->getCmd() << " : " << (*it)->getPID() << " " << difftime(time(nullptr),(*it)->getTime()) <<" secs (stopped)" << endl;
                }
            ++it;
        }
    }
}

void JobsList::killAllJobs() {
        removeFinishedJobs();

        if(!jobs_list->empty())
        {
            auto it=jobs_list->begin();
            while(it != jobs_list->end())
            {
                cout<<(*it)->getPID()<<": "<<(*it)->getCmd()<<endl;
                if(killpg((*it)->getPID(), SIGKILL)==-1)
                {
                    perror("smash error: kill failed");
                    return;
                }
                delete(*it);
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
                delete(*temp);
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
                    delete(*it);
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

JobsList::JobEntry *JobsList::getLastJob() {

        if(!jobs_list->empty())
        {
            return jobs_list->back();
        }

        return nullptr;
    }


JobsList::JobEntry *JobsList::getLastStoppedJob() {
    JobEntry* last_stopped = nullptr;
    int last_jobid = 0;

    if(!jobs_list->empty())
    {
        auto it=jobs_list->begin();
        while(it != jobs_list->end())
        {
            if(((*it)->getJobId() > last_jobid) && (!(*it)->getActiveStatus()))
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

int JobsList::getNumOfJobs() {
    int num = 0;
    removeFinishedJobs();
    if(!jobs_list->empty())
    {
        auto it = jobs_list->begin();
        while(it != jobs_list->end())
        {
            num++;
            ++it;
        }
    }

    return num;
}

void JobsList::killAllJobNotPrint() {
    removeFinishedJobs();

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
            delete(*it);

            ++it;
        }
    }
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

void JobsList::JobEntry::setJobId(int id) {
this->job_id=id;
}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute(){
    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().printJobsList();
    }


KillCommand::KillCommand(const char *cmd_line):BuiltInCommand(cmd_line){}

// check if it is only digits
bool onlyDigits(const std::string &str){return str.find_first_not_of("-0123456789") == std::string::npos;}

// helper func for checking input
bool chackInputForKill(KillCommand *k){
    if (k->getArgc()==1) return false;
    if(k->getArgc()>3) return false;
    bool first_args = onlyDigits(k->getArgs()[1]);
    if(!first_args) return false;
    bool second_args = onlyDigits(k->getArgs()[2]);
    if(!second_args)  return false;
    int signal_number = stoi(k->getArgs()[1]);
    signal_number = signal_number*(-1);
    if((signal_number <= 0) || (signal_number >= 32)) return false;
    return true;
}

void KillCommand::execute(){

    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().removeFinishedJobs(); //clean jobs from unfinished
    bool valid_input = chackInputForKill(this); //validating input
    if(!valid_input){
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(smash.getJobsListRef().getJobById(stoi(this->getArgs()[2])) == nullptr) //validating job existence
    {
        std::cerr << "smash error: kill: job-id " << stoi(this->getArgs()[2]) << " does not exist" << endl;
        return;
    }
    int signal_number = stoi(this->getArgs()[1]);
    signal_number = signal_number*(-1);
    pid_t pid = smash.getJobsListRef().getJobById(stoi(this->getArgs()[2]))->getPID();
    if(killpg(pid, signal_number) == -1){ //check if signal worked
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << signal_number << " was sent to pid " << pid << endl;
    smash.getJobsListRef().removeFinishedJobs(); //clean jobs list

}


//ForegroundCommand::ForegroundCommand(const char *cmdLine, const char *cmd_line, JobsList *jobs)
//        : BuiltInCommand(cmdLine) {}

bool chackInputForFg(ForegroundCommand *k){
    if(k->getArgc()>2) return false;
    if(k->getArgc()==2){
        bool first_args = onlyDigits(k->getArgs()[1]);
        if(!first_args) return false;
    }
    return true;
}

void ForegroundCommand::execute(){
    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().removeFinishedJobs();
    bool valid_input = chackInputForFg(this);
    if(!valid_input){
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    JobsList::JobEntry* job;
    pid_t pid;
    string cmd;

    if(this->getArgc()==2){
        int job_number = stoi(this->getArgs()[1]); //the relevant job number

        if(smash.getJobsListRef().getJobById(job_number)== nullptr){ //check if job exist
            cerr << "smash error: fg: job-id " << job_number << " does not exist" << endl;
            return;

        }
        //job exists:
        job=smash.getJobsListRef().getJobById(job_number);

    }
    //if only 1 argument
    else{
        if(smash.getJobsListRef().jobs_list->empty()){
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        job=smash.getJobsListRef().getLastJob();
    }
    pid = job->getPID();
    cmd = job->getCmd();
    cout << cmd << " : " << pid << endl;
    //set current job pid and cmd for execution
    char tmp_cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(tmp_cmd, cmd.c_str());
    smash.setCurrPid(pid);
    smash.setCurrCmd(tmp_cmd);
    //remove job from list
    ///smash.getJobsListRef().removeJobById(job->getJobId());  ///reomved this line to keep job id at possible re-entry

    //sending signals todo should we handle -1 status error?

    killpg(pid, SIGCONT);
    waitpid(pid, nullptr, WUNTRACED);

    //reset current cmd and pid
    SmallShell::getInstance().setCurrPid(-1);
    SmallShell::getInstance().setCurrCmd("");

}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

bool chackInputForBg(BackgroundCommand *k){
    if(k->getArgc()>2) return false;
    if(k->getArgc()==2){
        bool first_args = onlyDigits(k->getArgs()[1]);
        if(!first_args) return false;
    }
    return true;
}

void BackgroundCommand::execute(){
    SmallShell &smash = SmallShell::getInstance();
    smash.getJobsListRef().removeFinishedJobs();
    bool valid_input = chackInputForBg(this);
    if(!valid_input){
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    JobsList::JobEntry* job;
    pid_t pid;
    string cmd;

    if(this->getArgc()==2){
        int job_number = stoi(this->getArgs()[1]); //the relevant job number

        if(smash.getJobsListRef().getJobById(job_number)== nullptr){ //check if job exist
            cerr << "smash error: bg: job-id " << job_number << " does not exist" << endl;
            return;
        }
        //job exists and running:
        job=smash.getJobsListRef().getJobById(job_number);
        if(job->getActiveStatus()){
            cerr << "smash error: bg: job-id " << job_number << " is already running in the background" << endl;
            return;
        }

    }
        //if only 1 argument
    else{
        if(smash.getJobsListRef().jobs_list->empty()){
            cerr << "smash error: bg: jobs list is empty" << endl;
            return;
        }
        job=smash.getJobsListRef().getLastStoppedJob();
        if(job == nullptr){
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
    pid = job->getPID();
    cmd = job->getCmd();
    cout << cmd << " : " << pid << endl;
    //set current job pid and cmd for execution
    char tmp_cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(tmp_cmd, cmd.c_str());
    smash.setCurrPid(pid);
    smash.setCurrCmd(tmp_cmd);
    //change active status to true
    job->setActiveStatus(true);

    //sending signals todo should we handle -1 status error?

    killpg(pid, SIGCONT);

    //reset current cmd and pid
    SmallShell::getInstance().setCurrPid(-1);
    SmallShell::getInstance().setCurrCmd("");

}

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {};

void RedirectionCommand::prepare() {
    ///check if redirecton command is append(>>) or regular(>)
    if(string(getOgCmdLine()).find(">>")!=string::npos)
        append = true;


    ///extract command into redir_args[0]
    string redir_cmd(getOgCmdLine());

    std::size_t pos = redir_cmd.find(">");

    redir_args[0] = _trim(redir_cmd.substr(0, pos));


    ///discard & from command
    if(redir_args[0] != "") {
        unsigned int idx = redir_args[0].find_last_not_of(WHITESPACE);
        if(idx != string::npos && redir_args[0][idx] == '&')
        redir_args[0][idx] = ' ';
    }

    ///extract file name into redir_args[1] and open the file
    if(append) {
        redir_args[1] = _trim(redir_cmd.substr(pos+2));
        new_fd = open(redir_args[1].c_str(), O_CREAT | O_WRONLY | O_APPEND , 0777);
    } else {
        redir_args[1] = _trim(redir_cmd.substr(pos+1));
        new_fd = open(redir_args[1].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0777);
    }
    if(new_fd == -1) {
        perror("smash error: open failed");
        return;
    }

    ///switch stdout to redir_args[1]
    stdout_backup = dup(1);
    if(stdout_backup == -1) {
        perror("smash error: dup failed");
        return;
    }

    if(dup2(new_fd, 1) == -1) {
        perror("smash error: dup failed");
        return;
    }

}

void RedirectionCommand::execute() {
    ///verify open was successful
    if(new_fd == -1)
        return;

    ///create the redirected command stored in redir_args[0]
    Command *cmd = SmallShell::getInstance().CreateCommand(redir_args[0].c_str());
    cmd->setOgCmdLine(redir_args[0].c_str());

    if(cmd) {
        cmd->prepare();
        cmd->execute();
        cmd->cleanup();
        delete cmd;
    }
}

void RedirectionCommand::cleanup() {
    ///verify open was successful
    if(new_fd == -1)
        return;

    ///restore original stdout
    if(dup2(stdout_backup, 1) == -1) {
        perror("smash error: dup failed");
        return;
    }

    if(close(stdout_backup) == -1) {
        perror("smash error: close failed");
        return;
    }

    if(close(new_fd) == -1)
        perror("smash error: close failed");
}


void PipeCommand::prepare() {
    ///check if pipe write channel receives stdout(|) or stderr(|&)
    if(string(getOgCmdLine()).find("|&")!=string::npos)
        toStderr = true;


    ///extract command1 into redir_args[0]
    string redir_cmd(getOgCmdLine());

    std::size_t pos = redir_cmd.find("|");
    pipe_args[0] = _trim(redir_cmd.substr(0, pos));

    if(toStderr) {
        pipe_args[1] = _trim(redir_cmd.substr(pos+2));
    } else {
        pipe_args[1] = _trim(redir_cmd.substr(pos+1));
    }

    ///discard & from commands
    if(pipe_args[0] != "") {
        unsigned int idx = pipe_args[0].find_last_not_of(WHITESPACE);
        if(idx != string::npos && pipe_args[0][idx] == '&')
        pipe_args[0][idx] = ' ';
    }
    if(pipe_args[1] != "") {
        unsigned int idx = pipe_args[1].find_last_not_of(WHITESPACE);
        if(idx != string::npos && pipe_args[1][idx] == '&')
        pipe_args[1][idx] = ' ';
    }
}


PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line) {};
void PipeCommand::execute() {

    if(pipe(pipe_fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    ///first child - writing pipe_args[0] stdout/stderr to pipe
    pid_t pid1 = fork();
    if(pid1 == -1) {
        perror("smash error: pid failed");
        return;
    }
    if(pid1 == 0) {
        if(dup2(pipe_fd[1], 1) == -1) {
            perror("smash error: dup failed");
            exit(1);
        }
        if(close(pipe_fd[0]) == -1) {
            perror("smash error: close failed");
            exit(1);
        }
        SmallShell::getInstance().executeCommand(pipe_args[0].c_str());
        if(close(1) == -1){
            perror("smash error: close failed");
            exit(1);
        }
        if(close(pipe_fd[1]) == -1) {
            perror("smash error: close failed");
            exit(1);
        }
        exit(0);
    }

    ///second child - reading pipe_args[1] stdin from pipe
    pid_t pid2 = fork();
    if(pid2 == -1) {
        perror("smash error: pid failed");
        return;
    }
    if(pid2 == 0) {
        if(dup2(pipe_fd[0], 0) == -1) {
            perror("smash error: dup failed");
            exit(1);
        }
        if(close(pipe_fd[1]) == -1) {
            perror("smash error: close failed");
            exit(1);
        }
        SmallShell::getInstance().executeCommand(pipe_args[1].c_str());
        if(close(0) == -1) {
            perror("smash error: close failed");
            exit(1);
        }
        if(close(pipe_fd[0]) == -1) {
            perror("smash error: close failed");
            exit(1);
        }
        exit(0);
    }

    ///close each pipe end after the appropriate child exits
    if(waitpid(pid1,nullptr,WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    if(close(pipe_fd[1]) == -1) {
        perror("smash error: close failed");
        return;
    }
    if(waitpid(pid2,nullptr,WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    if(close(pipe_fd[0]) == -1) {
        perror("smash error: close failed");
        return;
    }
}


HeadCommand::HeadCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void HeadCommand::execute() {
    ///verify there are at least 2 of arguments ([-N] and\or <file>)
    if(this->getArgs()[1] == NULL) {
        std::cerr << "smash error: head: not enough arguments" << endl;
        return;
    }

    ///verify there are no more than 3 arguments (head [-N] <file>)
    if(this->getArgs()[3] != NULL) {
        std::cerr << "smash error: head: too many argumens" << endl;
        return;
    }

    ///verify -N input format
    if(this->getArgs()[2] != NULL) {
        if(!onlyDigits(getArgs()[1]) || getArgs()[1][0] != '-') {
            std::cerr << "smash error: head: invalid arguments" << endl;
            return;
        }
        int num = stoi(this->getArgs()[1]);
        numOfLines = -1*num;
        N_flag = true;
    }

    ///set cmd as original command
    string cmd(_trim(getOgCmdLine()));

    ///discard "head" from cmd
    int pos = cmd.find_first_of(WHITESPACE);
    cmd = _trim(cmd.substr(pos));

    ///discard "-N" flag from cmd
    if(N_flag) {
        pos = cmd.find_first_of(WHITESPACE);
        cmd = _trim(cmd.substr(pos));
    }

    ///open file at address <file>, if exists
    new_fd = open(cmd.c_str(), O_RDONLY, 0777);
    if(new_fd == -1) {///} && errno == ENOENT) {
        perror("smash error: open failed");
        return;
    }


    char buffer = '\0';
    int n=0;    //printed lines counter
    int ret=1;  //monitor read() return values

    ///while not at EOF or have reached number of requested line
    while(ret!=0 || n<numOfLines) {

        ///read next char frome file
        ret = read(new_fd, (void*)(&buffer), 1);
        if(ret == -1) {
            perror("smash error: read failed");
            return;
        }

        ///check for EOF
        if(ret == 0)
            break;

        ///check for end of line
        if(buffer == '\n')
            n++;

        ///print next char frome file
        if(write(1, (void*)(&buffer), 1) == -1) {
            perror("smash error: write failed");
            return;
        }

        ///check if reached number of requested line
        if(n == numOfLines)
            break;
    }

    if(close(new_fd) == -1) {
        perror("smash error: close failed");
        return;
    }
}
