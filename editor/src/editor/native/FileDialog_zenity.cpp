// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileDialog.h"
#include <memory>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
extern "C" { extern char **environ; }

std::string FileDialog::chooseFile()
{
    std::vector<std::string> args;
    args.reserve(16);
    args.push_back("zenity");
    args.push_back("--file-selection");
    if (mode_ == Mode::Save)
        args.push_back("--save");
    if (!title_.empty())
        args.push_back("--title=" + title_);
    if (!path_.empty())
        args.push_back("--filename=" + path_);
    for (const Filter& filter : filters_) {
        std::string arg = "--file-filter=" + filter.name + " |";
        for (const std::string& pattern : filter.patterns) {
            arg.push_back(' ');
            arg.append(pattern);
        }
        args.push_back(std::move(arg));
    }

    ///
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (size_t i = 0, n = args.size(); i < n; ++i)
        argv.push_back(const_cast<char*>(args[i].c_str()));
    argv.push_back(nullptr);

    ///
    struct SpawnData {
        SpawnData()
        {
            if (posix_spawnattr_init(&attr) != 0)
                return;
            if (posix_spawn_file_actions_init(&fileact) != 0) {
                posix_spawnattr_destroy(&attr);
                return;
            }
            valid = true;
        }
        ~SpawnData()
        {
            if (valid) {
                posix_spawn_file_actions_destroy(&fileact);
                posix_spawnattr_destroy(&attr);
            }
        }
        operator bool() const
        {
            return valid;
        }
        posix_spawnattr_t attr;
        posix_spawn_file_actions_t fileact;
        bool valid = false;
    };

    ///
    struct Pipe {
        Pipe()
        {
            valid = pipe(fd) == 0;
        }
        ~Pipe()
        {
            if (fd[0] != -1)
                close(fd[0]);
            if (fd[1] != -1)
                close(fd[1]);
        }
        operator bool() const
        {
            return valid;
        }
        int fd[2] = { -1, -1 };
        bool valid = false;
    };

    ///
    SpawnData spawn;
    if (!spawn)
        return {};

    Pipe pipe;
    if (!pipe)
        return {};

    if (posix_spawn_file_actions_adddup2(&spawn.fileact, pipe.fd[1], 1) != 0)
        return {};

    pid_t pid;
    if (posix_spawnp(&pid, argv[0], &spawn.fileact, &spawn.attr, argv.data(), environ) != 0)
        return {};

    close(pipe.fd[1]);
    pipe.fd[1] = -1;

    std::string result;
    result.reserve(1024);

    char buffer[1024];
    size_t count;
    while ((count = read(pipe.fd[0], buffer, sizeof(buffer))) > 0)
        result.append(buffer, count);

    int status;
    if (waitpid(pid, &status, 0) == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return {};

    if (!result.empty() && result.back() == '\n')
        result.pop_back();

    return result;
}
