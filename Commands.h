#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;

class Command {
// TODO: Add your data members
  const char* cmd_line;
  char* args[COMMAND_MAX_ARGS] = {NULL};
  bool background = false;
 public:
  Command(const char* cmd_line);
  virtual ~Command(){};
  virtual void execute() = 0;
  virtual void prepare();
  virtual void cleanup();
  // TODO: Add your extra methods if needed
  const char* getCmdLine() {return cmd_line;}
  char** getArgs() {return args;}
  void setAsBackground() {background = true;}
  bool isBackground() {return background;}
  bool isForeground() {return !background;}
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
 public:
// TODO: Add your data members public:
    std::string* prevPwd;
  ChangeDirCommand(const char* cmd_line, std::string* prevPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList {
 public:
  class JobEntry {
      int job_id;
      pid_t job_pid;
      time_t start_time;
      bool active_status;
      string command;
  public:
      JobEntry(string cmd, pid_t pid, int job_id, bool active_status = false) //Constructor
      {
          command = cmd;
          job_id = job_id;
          job_pid = pid;
          start_time = time(nullptr);
          active_status= active_status;
      }
      ~JobEntry();
      pid_t getPID();
      int getJobId();
      bool getActiveStatus();
      void setActiveStatus(bool status);
      string getCmd();
      time_t getTime();
      void resetTheTime();
  };

    list<JobEntry*>* jobs_list;
    int max_id;
 // TODO: Add your data members
 public:

    JobsList(){
        this->max_id = 0;
        this->jobs_list = new list<JobEntry*>();

    };
    ~JobsList();
//  void addJob(Command* cmd, bool isStopped = false) const;
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int getMaxId();
  void setMaxId(int new_id);
  void addJob(string cmd, pid_t pid, bool active_status = true);
  // TODO: Add extra methods or modify exisitng ones as needed
    JobEntry *getJobByPid(pid_t jobPid);
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char *cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class HeadCommand : public BuiltInCommand {
 public:
  HeadCommand(const char* cmd_line);
  virtual ~HeadCommand() {}
  void execute() override;
};


class SmallShell {
 private:
 // TODO: Add your data members
    std::string prompt = "smash";
    std::string prevPwd = "";
    bool quit = false;
    JobsList jobs;
    pid_t smashPid;
    pid_t currPid;
    string currCmd;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  std::string getPrompt() {return this->prompt;}
  void setPrompt(std::string newPrompt) {this->prompt = newPrompt;}
  std::string* getPrevPwd() {return &prevPwd;}
  bool quitSmash() {return quit;}
  void setToQuit() {quit = true;}

  //jobs related
  JobsList& getJobsListRef();
  JobsList getJobList();
  pid_t getCurrPid();
  pid_t getSmashPid();

};


class ChangePromptCommand : public BuiltInCommand {
  SmallShell* smash;
 public:
  ChangePromptCommand(SmallShell* smash, const char* cmd_line);
  virtual ~ChangePromptCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
  JobsList* jobs;
  SmallShell* smash;
 public:
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs, SmallShell* smash);
  virtual ~QuitCommand() {}
  void execute() override;
};

#endif //SMASH_COMMAND_H_
