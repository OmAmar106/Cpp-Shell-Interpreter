#include "jobs.h"

#include <algorithm>
#include <cerrno>
#include <sys/wait.h>

using namespace std;

static int current_job = 0;
static int previous_job = 0;

static vector<int> job;
static vector<int> process;
static vector<string> com;
static vector<int> job_history;

static void update_job_markers() {
    current_job = 0;
    previous_job = 0;

    for (int i = (int)job_history.size() - 1; i >= 0; i--) {
        int id = job_history[i];

        if (find(job.begin(), job.end(), id) == job.end()) {
            continue;
        }

        if (current_job == 0) {
            current_job = id;
        } else {
            previous_job = id;
            break;
        }
    }
}

static void erase_job(int i) {
    process.erase(process.begin() + i);
    job.erase(job.begin() + i);
    com.erase(com.begin() + i);
    update_job_markers();
}

int add_job(pid_t pid, const string& command) {
    int next_job = 1;

    for (int id : job) {
        next_job = max(next_job, id + 1);
    }

    job.push_back(next_job);
    process.push_back(pid);
    com.push_back(command);
    job_history.push_back(next_job);

    previous_job = current_job;
    current_job = next_job;

    return next_job;
}

string check_jobs() {
    string st;

    for (int i = 0; i < (int)process.size();) {
        int status = 0;
        pid_t ret = waitpid(process[i], &status, WNOHANG);

        if (ret > 0 || (ret == -1 && errno == ECHILD)) {
            int finished_job = job[i];

            string marker = finished_job == current_job ? "+" :
                                 finished_job == previous_job ? "-" : " ";

            if (!st.empty()) st += "\n";
            st += "[" + to_string(finished_job) + "]" + marker +
                  "  Done                    " +
                  com[i].substr(0, com[i].size() - 1);

            erase_job(i);
        } else {
            i++;
        }
    }

    return st;
}

pair<string, int> builtin_jobs(const vector<string>&) {
    string st;

    for (int i = 0; i < (int)process.size();) {
        int status = 0;
        pid_t ret = waitpid(process[i], &status, WNOHANG);

        int id = job[i];
        string marker = id == current_job ? "+" :
                             id == previous_job ? "-" : " ";

        if (!st.empty()) st += "\n";

        if (ret > 0 || (ret == -1 && errno == ECHILD)) {
            st += "[" + to_string(id) + "]" + marker +
                  "  Done                    " +
                  com[i].substr(0, com[i].size() - 1);

            erase_job(i);
        } else {
            st += "[" + to_string(id) + "]" + marker +
                  "  Running                 " + com[i];
            i++;
        }
    }

    return {st, 2};
}
