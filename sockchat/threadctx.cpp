#include "cthread.h"

ThreadContext::Connection::Connection(sc::Socket *sock) {
    this->sock = sock;
}

ThreadContext::ThreadContext(sc::Socket *sock) {
    this->pendingConns.push_back(Connection(sock));
}

void ThreadContext::Finish() {
    for(auto i = this->conns.begin(); i != this->conns.end(); )
        i = CloseConnection(i);
    this->done = true;
}

bool ThreadContext::IsDone() {
    return this->done;
}

void ThreadContext::PushSocket(sc::Socket *sock) {
    this->_pendingConns.lock();
    this->pendingConns.push_back(sock);
    this->_pendingConns.unlock();
}

void ThreadContext::HandlePendingConnections() {
    this->_pendingConns.lock();
    
    for(auto i = this->pendingConns.begin(); i != this->pendingConns.end(); ) {
        i->sock->SetBlocking(false);
        this->conns.push_back(*i);
        i = pendingConns.erase(i);
    }
    
    this->_pendingConns.unlock();
}

std::vector<ThreadContext::Connection>::iterator ThreadContext::CloseConnection(std::vector<Connection>::iterator i) {
    i->sock->Close();
    delete i->sock;

    if(i->u != NULL && i->uconn != NULL) {
        std::cout << i->u->GetOriginalUsername() << " disconnected." << std::endl;
        i->u->UnhookConnection(i->uconn);
        delete i->uconn;
    }

    return this->conns.erase(i);
}

std::vector<ThreadContext*> Threads::conns;
std::mutex Threads::_conns;

void Threads::Init() {
    conns = std::vector<ThreadContext*>();
}

void Threads::AddConnection(sc::Socket *sock) {
    _conns.lock();
    
    /*
    if(conns.count(sock->GetIPAddress()) == 0) {
        auto tmp = new ThreadContext(sock);
        conns[sock->GetIPAddress()] = tmp;
        std::thread(connectionThread, tmp).detach();
    } else
        conns[sock->GetIPAddress()]->PushSocket(sock);
    */

    if(conns.size() == 0) {
        auto tmp = new ThreadContext(sock);
        conns.push_back(tmp);
        std::thread(connectionThread, tmp).detach();
    } else
        conns[0]->PushSocket(sock);
    
    _conns.unlock();
}

void Threads::RemoveContext(ThreadContext *ctx) {
    if(!ctx->IsDone()) return;
    
    _conns.lock();
    
    for(auto i = conns.begin(); i != conns.end(); ++i) {
        if(*i == ctx) {
            conns.erase(i);
            break;
        }
    }

    delete ctx;
    
    _conns.unlock();
}