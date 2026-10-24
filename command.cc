#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <vector>
#include <string>
#include <iostream>
#include "command.h"
#include "tokenizer.h"

void parse(std::vector<Token> &tokens);
// PROTOTYPES (later steps add lines below this one)
static bool runBuiltin(Command *c);
static void waitForChild(pid_t pid);
static void runProgram(SimpleCommand *s);

static bool isWord(const Token &t)
{
    return t.type == TOKEN_COMMAND || t.type == TOKEN_ARGUMENT;
}

static void parseError(const char *msg)
{
    fprintf(stderr, "myshell: syntax error: %s\n", msg);
    Command::_currentCommand.clear();
}

// BEGIN parse
void parse(std::vector<Token> &tokens)
{
    if (tokens.size() <= 1)
        return;

    Command &cmd = Command::_currentCommand;
    SimpleCommand *simple = NULL;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        Token &t = tokens[i];

        switch (t.type)
        {
        case TOKEN_COMMAND:
        case TOKEN_ARGUMENT:
            if (!simple)
            {
                simple = new SimpleCommand();
                cmd.insertSimpleCommand(simple);
            }
            simple->insertArgument(strdup(t.value.c_str()));
            break;

        case TOKEN_PIPE:
            if (!simple)
            {
                parseError("missing command before '|'");
                return;
            }
            if (!isWord(tokens[i + 1]))
            {
                parseError("missing command after '|'");
                return;
            }
            simple = NULL;
            break;

        case TOKEN_REDIRECT:
        case TOKEN_APPEND:
        case TOKEN_INPUT:
        case TOKEN_ERROR:
        case TOKEN_REDIRECT_AND_ERROR:
        {
            if (!isWord(tokens[i + 1]))
            {
                parseError("missing file name after redirection");
                return;
            }
            char *file = strdup(tokens[i + 1].value.c_str());

            if (t.type == TOKEN_INPUT)
            {
                if (cmd._inputFile)
                {
                    free(file);
                    parseError("ambiguous input redirection");
                    return;
                }
                cmd._inputFile = file;
            }
            else if (t.type == TOKEN_ERROR)
            {
                if (cmd._errFile)
                {
                    free(file);
                    parseError("ambiguous error redirection");
                    return;
                }
                cmd._errFile = file;
            }
            else
            {
                if (cmd._outFile)
                {
                    free(file);
                    parseError("ambiguous output redirection");
                    return;
                }
                cmd._outFile = file;
                cmd._append = (t.type != TOKEN_REDIRECT);
                if (t.type == TOKEN_REDIRECT_AND_ERROR)
                {
                    if (cmd._errFile)
                    {
                        parseError("ambiguous error redirection");
                        return;
                    }
                    cmd._errFile = strdup(file);
                    cmd._out_error = 1;
                }
            }
            i++;
            break;
        }

        case TOKEN_BACKGROUND:
            if (tokens[i + 1].type != TOKEN_EOF)
            {
                parseError("'&' must be the last token");
                return;
            }
            if (!simple)
            {
                parseError("missing command before '&'");
                return;
            }
            cmd._background = 1;
            break;

        case TOKEN_EOF:
            if (!simple)
            {
                parseError("missing command");
                return;
            }
            break;
        }
    }

    cmd.execute();
}
// END parse

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }
    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;
    _numberOfArguments++;
}

Command::Command()
{
    _numberOfSimpleCommands = 0;
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _append = 0;
    _background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
            _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }
    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
            free(_simpleCommands[i]->_arguments[j]);

        free(_simpleCommands[i]->_arguments);
        delete _simpleCommands[i];
    }

    if (_outFile)
        free(_outFile);
    if (_inputFile)
        free(_inputFile);
    if (_errFile)
        free(_errFile);

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _append = 0;
    _background = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        printf("\n");
    }

    printf("\n");
    printf("  Output       Input        Error        Err&Out      Append  Background\n");
    printf("  ------------ ------------ ------------ ------------ ------- ----------\n");
    printf("  %-12s %-12s %-12s %-12s %-7s %-10s\n",
           _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default",
           _errFile ? _errFile : "default",
           (_out_error && _errFile) ? _errFile : "default",
           _append ? "YES" : "NO",
           _background ? "YES" : "NO");
    printf("\n\n");
}







// BEGIN redirect
static int openOrDie(const char *path, int flags)
{
    int fd = open(path, flags, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "myshell: %s: %s\n", path, strerror(errno));
        _exit(1);
    }
    return fd;
}

static void applyRedirections(Command *c, bool first, bool last)
{
    if (first && c->_inputFile)
    {
        int fd = openOrDie(c->_inputFile, O_RDONLY);
        dup2(fd, 0);
        close(fd);
    }

    if (last && c->_outFile)
    {
        int flags = O_WRONLY | O_CREAT | (c->_append ? O_APPEND : O_TRUNC);
        int fd = openOrDie(c->_outFile, flags);
        dup2(fd, 1);
        if (c->_out_error)
            dup2(fd, 2);
        close(fd);
    }

    if (c->_errFile && !c->_out_error)
    {
        int fd = openOrDie(c->_errFile, O_WRONLY | O_CREAT | O_TRUNC);
        dup2(fd, 2);
        close(fd);
    }
}
// END redirect


// BEGIN execute
void Command::execute()
{
    int n = _numberOfSimpleCommands;
    if (n == 0)
    {
        clear();
        return;
    }

    if (runBuiltin(this))
    {
        clear();
        return;
    }

    std::vector<int> fds(2 * (n - 1));
    for (int i = 0; i < n - 1; i++)
    {
        if (pipe(&fds[2 * i]) < 0)
        {
            perror("myshell: pipe");
            for (int k = 0; k < 2 * i; k++)
                close(fds[k]);
            clear();
            return;
        }
    }

    fflush(stdout);
    std::vector<pid_t> pids;

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("myshell: fork");
            break;
        }

        if (pid == 0)
        {
            if (i > 0)
                dup2(fds[2 * (i - 1)], 0);
            if (i < n - 1)
                dup2(fds[2 * i + 1], 1);

            for (size_t k = 0; k < fds.size(); k++)
                close(fds[k]);

            applyRedirections(this, i == 0, i == n - 1);
            runProgram(_simpleCommands[i]);
        }

        pids.push_back(pid);
    }

    for (size_t k = 0; k < fds.size(); k++)
        close(fds[k]);

    if (_background)
    {
        if (!pids.empty())
            printf("[background] pid %d\n", (int)pids.back());
    }
    else
    {
        for (size_t k = 0; k < pids.size(); k++)
            waitForChild(pids[k]);
    }

    clear();
}
// END execute

// BEGIN sigchld
static char g_logPath[1024] = "myshell.log";

static void logChild(pid_t pid)
{
    int fd = open(g_logPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return;

    char line[128];
    int len = snprintf(line, sizeof(line), "Child process %d terminated\n", (int)pid);
    if (write(fd, line, len) < 0)
    {
    }
    close(fd);
}

static void childHandler(int sig)
{
    (void)sig;
    int saved = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        logChild(pid);
    errno = saved;
}
// END sigchld

// BEGIN builtin
static bool runBuiltin(Command *c)
{
    if (c->_numberOfSimpleCommands != 1)
        return false;

    char **args = c->_simpleCommands[0]->_arguments;

    if (strcmp(args[0], "exit") == 0)
    {
        printf("Good bye!!\n");
        c->clear();
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0)
    {
        const char *dir = args[1];
        if (dir == NULL)
            dir = getenv("HOME");

        if (dir == NULL || chdir(dir) != 0)
            fprintf(stderr, "myshell: cd: %s: %s\n",
                    dir ? dir : "(HOME not set)", strerror(errno));
        return true;
    }

    return false;
}
// END builtin

// BEGIN wait
static void waitForChild(pid_t pid)
{
    int status;
    pid_t r;
    do
    {
        r = waitpid(pid, &status, 0);
    } while (r < 0 && errno == EINTR);

    if (r > 0)
        logChild(r);
}
// END wait

// BEGIN runprogram
static void runProgram(SimpleCommand *s)
{
    signal(SIGINT, SIG_DFL);

    execvp(s->_arguments[0], s->_arguments);

    if (errno == ENOENT)
        fprintf(stderr, "myshell: %s: command not found\n", s->_arguments[0]);
    else
        fprintf(stderr, "myshell: %s: %s\n", s->_arguments[0], strerror(errno));
    _exit(127);
}
// END runprogram

// BEGIN prompt
void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);

        if (!std::getline(std::cin, input))
        {
            printf("\n");
            break;
        }
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    signal(SIGINT, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = childHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    if (getcwd(g_logPath, sizeof(g_logPath) - 16) != NULL)
        strcat(g_logPath, "/myshell.log");

    Command::_currentCommand.prompt();
    return 0;
}
// END prompt
