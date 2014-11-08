#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <QObject>
#include <QSocketNotifier>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define SIGNALHANDLER_WIN32
#else
#define SIGNALHANDLER_POSIX
#endif

class SignalHandler : public QObject
{
    Q_OBJECT
public:
    explicit SignalHandler(QObject *parent = 0, int mask = DEFAULT_SIGNALS);
    virtual ~SignalHandler();

    enum SIGNALS
    {
        SIG_UNHANDLED   = 0,    // Physical signal not supported by this class
        SIG_NOOP        = 1,    // The application is requested to do a no-op (only a target that platform-specific signals map to when they can't be raised anyway)
        SIG_INT         = 2,    // Control+C (should terminate but consider that it's a normal way to do so; can delay a bit)
        SIG_TERM        = 4,    // Control+Break (should terminate now without regarding the consquences)
        SIG_CLOSE       = 8,    // Container window closed (should perform normal termination, like Ctrl^C) [Windows only; on Linux it maps to SIG_TERM]
        SIG_RELOAD      = 16,   // Reload the configuration [Linux only, physical signal is SIGHUP; on Windows it maps to SIG_NOOP]
        DEFAULT_SIGNALS = SIG_INT | SIG_TERM | SIG_CLOSE,
    };
    static const int signalsCount = 6;

    bool handleSignal(int signal);

#ifdef SIGNALHANDLER_WIN32
    static BOOL WINAPI WIN32_handleFunc(DWORD);
    static int WIN32_physicalToLogical(DWORD);
    static DWORD WIN32_logicalToPhysical(int);
    static std::set<int> sig_registry;
#endif
#ifdef SIGNALHANDLER_POSIX
    static void POSIX_handleFunc(int);
    static int POSIX_physicalToLogical(int);
    static int POSIX_logicalToPhysical(int);
#endif //SIGNALHANDLER_POSIX

signals:
    void signalReceived(int signal);

public slots:
#ifdef SIGNALHANDLER_POSIX
    void POSIX_socketRecv();
#endif

private:
    int _mask;

#ifdef SIGNALHANDLER_POSIX
    static int POSIX_fd[2];
    QSocketNotifier *POSIX_ntf;
#endif
};

#endif // SIGNALHANDLER_H
