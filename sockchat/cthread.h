#ifndef CTHREADH
#define CTHREADH

#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <locale>
#include <time.h>
#include <mutex>
#include "socklib/context.h"
#include "socklib/socket.h"
#include "socklib/library.h"
#include "socklib/utils.h"

class ThreadContext {
public:
    struct Connection {
        sc::Socket *sock;
        sc::User *u = NULL;
        sc::User::Connection *uconn = NULL;
        bool initialized = false;

        Connection(sc::Socket *sock);
    };

    ThreadContext(sc::Socket *sock);

    void PushSocket(sc::Socket *sock);
    void HandlePendingConnections();

    std::vector<Connection>::iterator CloseConnection(std::vector<Connection>::iterator i);

    void Finish();
    bool IsDone();

    std::vector<Connection> conns = std::vector<Connection>();
private:
    std::vector<Connection> pendingConns = std::vector<Connection>();
    std::mutex _pendingConns;

    bool done = false;
};

class Threads {
    static std::vector<ThreadContext*> conns;
    static std::mutex _conns;
public:
    static void Init();
    static void AddConnection(sc::Socket *sock);
    static void RemoveContext(ThreadContext *ctx);
};

void connectionThread(ThreadContext *ctx);
bool interpretMessage(sc::Message msg, ThreadContext::Connection *conn);

#endif