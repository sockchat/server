#include "cthread.h"

void connectionThread(ThreadContext *ctx) {
    ctx->HandlePendingConnections();

    int status;
    std::string in;
    while(ctx->conns.size() > 0) {
        ctx->HandlePendingConnections();

        for(auto i = ctx->conns.begin(); i != ctx->conns.end(); ) {
            if(!i->initialized) {
                i->sock->SetBlocking(true);
                i->sock->SetTimeout(0, 10);
                
                if((status = i->sock->Recv(in, 3)) == 0) {
                    if(in == "GET") {
                        auto tmp = new sc::WebSocket(*(i->sock));
                        delete i->sock;
                        i->sock = tmp;

                        if(((sc::WebSocket*)i->sock)->Handshake(in))
                            i->initialized = true;
                        else {
                            i = ctx->CloseConnection(i);
                            continue;
                        }
                    } else if(in == "TCP") {
                        auto tmp = new sc::RawSocket(*(i->sock));
                        delete i->sock;
                        i->sock = tmp;
                        i->initialized = true;
                    }  else {
                        i = ctx->CloseConnection(i);
                        continue;
                    }
                } else if(status == -1) {
                    i = ctx->CloseConnection(i);
                    continue;
                }
                ++i;
            } else {
                if((status = i->sock->Recv(in)) == -1) {
                    auto sdf = i->sock->GetLastError();
                    i = ctx->CloseConnection(i);
                    continue;
                } else if(status == 0) {
                    auto msg = sc::Message(in);
                    if(msg.IsLegal()) {
                        if(!interpretMessage(msg, &*i)) {
                            i = ctx->CloseConnection(i);
                            continue;
                        }
                    }
                }

                if(i->uconn != NULL) {
                    while(i->uconn->CanPopMessage()) {
                        if(i->sock->Send(i->uconn->PopMessage().Get()) == -1) {
                            i = ctx->CloseConnection(i);
                            continue;
                        }
                    }
                }

                ++i;
            }
        }
    }

    ctx->Finish();
    Threads::RemoveContext(ctx);
}

bool boundMessage(sc::Message msg, uint16_t lower, uint16_t upper = 0) {
    uint16_t size = msg.Count();
    return size >= lower && (size <= upper || upper == 0);
}

bool interpretMessage(sc::Message msg, ThreadContext::Connection *conn) {
    sc::User *user = conn->u;
    auto args = msg.GetParts();

    if(msg.GetID() > 1 && user == NULL)
        return false;

    switch(msg.GetID()) {
        case P_JOIN:
            if(!boundMessage(msg, 2)) break;
            if(user != NULL) break;

            if(msg[0] == "0") {
                args = std::vector<std::string>(args.begin() + 1, args.end());

                sc::User u = sc::Context::Auth::Authenticate(args);
                if(u.IsValid()) {
                    user = conn->u = sc::Context::AddNewUser(u);
                    conn->uconn = new sc::User::Connection(conn->sock->GetIPAddress());
                    user->HookConnection(conn->uconn);

                    conn->uconn->PushMessage(sc::Message(P_JOIN, {"1", TOSTR(user->GetId()), user->GetUsername(), user->GetColor(), user->GetPermissions().Get(), "2000", "en"})); 
                    break;
                }

                conn->sock->Send(sc::Message(P_JOIN, {"0", "0"}).Get());
                return false;
            } else if(msg[0] == "1") {
                auto args = msg.GetParts();
                args = std::vector<std::string>(args.begin() + 1, args.end());

                // session based auth
            }
            break;
        case P_MSG:
            if(!boundMessage(msg, 1, 2)) break;

            if(msg.Count() == 2) {
                uint32_t channel;
                if(!sc::str::toInt<uint32_t>(msg[1], channel, false)) break;
                if(user->IsInChannel(channel)) {
                    sc::Context::GetChannelById(channel)->Broadcast(
                        sc::format::usermsg(*user, msg[0], channel)
                    );
                }
            } else {
                // channel contextless message
            }
            break;
        case P_SETTINGS:
            // settings
            break;

        // case 4 reserved

        case P_CHCHAN:
            if(!boundMessage(msg, 2, 3)) break;

            if(msg[0] == "1") {
                sc::Channel *channel;
                uint32_t channelId;
                if(!sc::str::toInt<uint32_t>(msg[1], channelId, false)) break;
                if(user->IsInChannel(channelId)) break;
                if((channel = sc::Context::GetChannelById(channelId)) == NULL) break;

                auto ret = (msg.Size() == 3)
                    ? channel->Join(user, "jchan", msg[2])
                    : channel->Join(user);

                if(ret == sc::Channel::JoinError::OK) {
                    auto logs = channel->Backlogs();
                    for(auto i = logs.begin(); i != logs.end(); ++i)
                        conn->uconn->PushMessage(*i);
                }
            } else if(msg[0] == "2") {
                sc::Channel *channel;
                uint32_t channelId;
                if(!sc::str::toInt<uint32_t>(msg[1], channelId, false)) break;
                if(!user->IsInChannel(channelId)) break;
                if((channel = sc::Context::GetChannelById(channelId)) == NULL) break;

                channel->Leave(user);
            }
            break;
    }

    return true;
}