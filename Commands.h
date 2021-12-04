#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <cstring>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;

class Command {
  const char* cmd_line;
  char og_cmd_line[COMMAND_ARGS_MAX_LENGTH];
  char* args[COMMAND_MAX_ARGS] = {NULL};
  bool background = false;
  int argc{};
 public:
  Command(const char* cmd_line);
  virtual ~Command(){};
  virtual void execute() = 0;
  virtual void prepare();
  virtual void cleanup();
  const char* getCmdLine() {return cmd_line;}
  char** getArgs() {return args;}
  void setAsBackground() {background = true;}
  bool isBackground() {return background;}
  bool isForeground() {return !background;}
  int getArgc();
  const char* getOgCmdLine() {return og_cmd_line;}
  void setOgCmdLine(const char* line) {strcpy(og_cmd_line, line);}
};

class BuiltInCommand : public Command {
    int argc();
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
    int argc();
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  string pipe_args[2];
  bool toStderr = false;
  int pipe_fd[2];
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
  void prepare() override;
};

class RedirectionCommand : public Command {
  string redir_args[2];
  bool append = false;
  int stdout_backup;
  int new_fd;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  void prepare() override;
  void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
 public:
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
      JobEntry(string cmd, pid_t pid, int job_id, bool active_status = true) //Constructor
      {
          command = cmd;
          job_id = job_id;
          job_pid = pid;
          start_time = time(nullptr);
          active_status=active_status;
      }
      ~JobEntry() = default;
      pid_t getPID();
      int getJobId();
      bool getActiveStatus();
      void setActiveStatus(bool status);
      void setJobId(int id);
      string getCmd();
      time_t getTime();
      void resetTheTime();
  };

    list<JobEntry*>* jobs_list;
    int max_id;
 public:

    JobsList(){
        this->max_id = 0;
        this->jobs_list = new list<JobEntry*>();

    };
    ~JobsList();
  void printJobsList();
  void killAllJobs();
  void killAllJobNotPrint();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  int getMaxId();
  int getNumOfJobs();
  void setMaxId(int new_id);
  void addJob(string cmd, pid_t pid, bool active_status = true);
    JobEntry *getJobByPid(pid_t jobPid);
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 public:
  KillCommand(const char *cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
    explicit ForegroundCommand(const char* cmd_line);
  ForegroundCommand(const char *cmdLine, const char *cmd_line, JobsList *jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 public:
    explicit BackgroundCommand(const char* cmd_line);
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class HeadCommand : public BuiltInCommand {
    int numOfLines = 10;
    bool N_flag = false;
    int new_fd;
 public:
  HeadCommand(const char* cmd_line);
  virtual ~HeadCommand() {}
  void execute() override;
};


class SmallShell {
 private:
    std::string prompt = "smash";
    std::string prevPwd = "";
    bool quit = false;
    JobsList jobs;
    pid_t smashPid;
    pid_t currPid = -1;
    string currCmd = "";
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
  string getCurrCmd() {return currCmd;};
  void setCurrPid(pid_t pid);
  void setCurrCmd(string cmd);

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
  QuitCommand(const char* cmd_line, JobsList* jobs, SmallShell* smash);
  virtual ~QuitCommand() {}
  void execute() override;
};

#endif //SMASH_COMMAND_H_
