#include "signalhandler.h"
#include "assert.h"

#ifdef SIGNALHANDLER_POSIX
#include <csignal>
#include <sys/socket.h>
#include <unistd.h>
#endif
#ifdef SIGNALHANDLER_WIN32
#include <windows.h>
#endif //SIGNALHANDLER_WIN32

#ifdef SIGNALHANDLER_POSIX
int SignalHandler::POSIX_fd[2];
#endif //SIGNALHANDLER_POSIX

// Singleton construction
SignalHandler* sig_handler(NULL);

SignalHandler::SignalHandler(QObject *parent, int mask) :
    QObject(parent), _mask(mask)
{
    assert(sig_handler == NULL);
    sig_handler = this;

#ifdef SIGNALHANDLER_WIN32
    SetConsoleCtrlHandler(WIN32_handleFunc, TRUE);
#endif
#ifdef SIGNALHANDLER_POSIX
    // Setup unix singal handling
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, POSIX_fd)) {
        qFatal("Failed to create POSIX Signal handling sockets");
    }
    POSIX_ntf = new QSocketNotifier(POSIX_fd[1], QSocketNotifier::Read, this);
    connect(POSIX_ntf, SIGNAL(activated(int)), this, SLOT(POSIX_socketRecv()));
#endif //SIGNALHANDLER_POSIX

    for (int i=0; i<signalsCount; i++) {
        int sigval = 1 << i;
        if (_mask & sigval) {
#ifdef SIGNALHANDLER_WIN32
            sig_registry.insert(sigval);
#endif
#ifdef SIGNALHANDLER_POSIX
            int sig = POSIX_logicalToPhysical(sigval);
            assert(signal(sig, SignalHandler::POSIX_handleFunc) != SIG_ERR);
#endif //SIGNALHANDLER_POSIX
        }
    }
}

SignalHandler::~SignalHandler()
{
#ifdef SIGNALHANDLER_WIN32
    SetConsoleCtrlHandler(WIN32_handleFunc, FALSE);
#endif
#ifdef SIGNALHANDLER_POSIX
    for (int i=0;i<signalsCount;i++) {
        int sigval = 1 << i;
        if (_mask & sigval) {
            signal(POSIX_logicalToPhysical(sigval), SIG_DFL);
        }
    }
#endif //SIGNALHANDLER_POSIX
}

bool SignalHandler::handleSignal(int signal)
{
    emit this->signalReceived(signal);

    return true;
}

#ifdef SIGNALHANDLER_WIN32
DWORD SignalHandler::WIN32_logicalToPhysical(int signal)
{
    switch (signal) {
        case SignalHandler::SIG_INT: return CTRL_C_EVENT;
        case SignalHandler::SIG_TERM: return CTRL_BREAK_EVENT;
        case SignalHandler::SIG_CLOSE: return CTRL_CLOSE_EVENT;
        default:
            return ~(unsigned int)0; // SIG_ERR = -1
    }
}
#endif
#ifdef SIGNALHANDLER_POSIX
int SignalHandler::POSIX_logicalToPhysical(int signal)
{
    switch (signal) {
        case SignalHandler::SIG_INT: return SIGINT;
        case SignalHandler::SIG_TERM: return SIGTERM;
        // In case the client asks for a SIG_CLOSE handler, accept and
        // bind it to a SIGTERM. Anyway the signal will never be raised
        case SignalHandler::SIG_CLOSE: return SIGTERM;
        case SignalHandler::SIG_RELOAD: return SIGHUP;
        default:
            return -1; // SIG_ERR = -1
    }
}
#endif //SIGNALHANDLER_POSIX


#ifdef SIGNALHANDLER_WIN32
int SignalHandler::WIN32_physicalToLogical(DWORD signal)
{
    switch (signal) {
        case CTRL_C_EVENT: return SignalHandler::SIG_INT;
        case CTRL_BREAK_EVENT: return SignalHandler::SIG_TERM;
        case CTRL_CLOSE_EVENT: return SignalHandler::SIG_CLOSE;
        default:
            return SignalHandler::SIG_UNHANDLED;
    }
}
#endif
#ifdef SIGNALHANDLER_POSIX
int SignalHandler::POSIX_physicalToLogical(int signal)
{
    switch (signal) {
        case SIGINT: return SignalHandler::SIG_INT;
        case SIGTERM: return SignalHandler::SIG_TERM;
        case SIGHUP: return SignalHandler::SIG_RELOAD;
        default:
            return SignalHandler::SIG_UNHANDLED;
    }
}
#endif //SIGNALHANDLER_POSIX



#ifdef SIGNALHANDLER_WIN32
BOOL WINAPI SignalHandler::WIN32_handleFunc(DWORD signal)
{
    if (sig_handler) {
        int signo = WIN32_physicalToLogical(signal);
        // The std::set is thread-safe in const reading access and we never
        // write to it after the program has started so we don't need to
        // protect this search by a mutex
        std::set<int>::const_iterator found = sig_registry.find(signo);
        if (signo != -1 && found != g_registry.end()) {
            return sig_handler->handleSignal(signo) ? TRUE : FALSE;
        }
        else {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
}
#endif
#ifdef SIGNALHANDLER_POSIX
void SignalHandler::POSIX_handleFunc(int signal)
{
    if (sig_handler) {
        int signo = POSIX_physicalToLogical(signal);
        write(POSIX_fd[0], &signo, sizeof(signo));
    }
}
#endif //SIGNALHANDLER_POSIX

#ifdef SIGNALHANDLER_POSIX
void SignalHandler::POSIX_socketRecv()
{
    POSIX_ntf->setEnabled(false);

    int signo;
    read(POSIX_fd[1], &signo, sizeof(signo));
    handleSignal(signo);

    POSIX_ntf->setEnabled(true);
}
#endif //SIGNALHANDLER_POSIX
