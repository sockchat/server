#include "socklib/context.h"

// *********** //
// sc::Context //
// *********** //

// STATIC MEMBERS //
sc::INI sc::Context::config;

uint64_t sc::Context::msgId;
std::mutex sc::Context::msgId_m;

std::vector<sc::Ban> sc::Context::bans;
std::mutex sc::Context::bans_m;

std::map<int64_t, sc::User*> sc::Context::users;
std::mutex sc::Context::users_m;

std::map<uint32_t, sc::Channel*> sc::Context::channelMapIds;
sc::imap<sc::Channel*> sc::Context::channelMapNames;
std::mutex sc::Context::channelMap_m;

// CLASS METHODS //
std::vector<sc::Ban>::iterator sc::Context::UnlinkBan(std::vector<Ban>::iterator i) {
    bans_m.lock();
    auto ret = bans.erase(i);
    bans_m.unlock();
    
    return ret;
}

void sc::Context::UnlinkUserFromMap(User *u, bool lockUsersMutex) {
    if(lockUsersMutex)
        users_m.lock();

    if(users[u->GetId()] == u)
        users.erase(u->GetId());

    for(auto i = u->channels.begin(); i != u->channels.end(); i = u->channels.begin())
        i->second->Leave(u, "leave");

    if(lockUsersMutex)
        users_m.unlock();
}

void sc::Context::UnlinkChannelFromMap(sc::Channel *c) {
    channelMap_m.lock();
    
    auto i = channelMapIds.find(c->GetId());
    if(i != channelMapIds.end() && i->second == c)
        channelMapIds.erase(i);

    auto j = channelMapNames.find(c->GetName());
    if(j != channelMapNames.end() && j->second == c)
        channelMapNames.erase(j);
    
    channelMap_m.unlock();
}

void sc::Context::AddChannelToMap(sc::Channel *c) {
    channelMap_m.lock();
    
    if(channelMapNames.count(c->GetName()) == 0) {
        channelMapNames[c->GetName()] = c;

        uint32_t i = 1;
        for(; channelMapIds.find(i) != channelMapIds.end(); i++);
        channelMapIds[i] = c;
        c->id = i;
    }
    
    channelMap_m.unlock();
}

void sc::Context::Init() {
    msgId = 1;
    bans = std::vector<Ban>();
    users = std::map<int64_t, User*>();
    channelMapIds = std::map<uint32_t, Channel*>();
    channelMapNames = sc::imap<Channel*>();

    config = sc::INI("sc_config.ini", {
        {"socket", {
            {"client_root", sc::INI::STRING},
            {"port",        sc::INI::INTEGER}
        }},
        {"structure", {
            {"backlog_length",  sc::INI::INTEGER},
            {"default_channel", sc::INI::STRING},
            {"chatbot_name",    sc::INI::STRING}
        }},
        {"limits", {
            {"max_conns_per_ip",        sc::INI::INTEGER},
            {"max_conns_per_user",      sc::INI::INTEGER},
            {"max_channel_name_length", sc::INI::INTEGER},
            {"max_username_length",     sc::INI::INTEGER},
            {"max_message_length",      sc::INI::INTEGER},
            {"max_idle_time",           sc::INI::INTEGER}
        }}
    });

    channelMapIds[0] = new Channel(config["structure"]["default_channel"]);
    channelMapIds[0]->id = 0;
    channelMapNames[channelMapIds[0]->GetName()] = channelMapIds[0];
}

void sc::Context::Broadcast(sc::Message msg) {
    users_m.lock();
    
    for(auto i = users.begin(); i != users.end(); ++i)
        i->second->Send(msg);
    
    users_m.unlock();
}

int64_t sc::Context::FetchNextGenericID(bool lockUsersMutex) {
    if(lockUsersMutex) users_m.lock();
    
    int64_t i = -3;
    for(;; i--) {
        if(users.find(i) == users.end())
            break;
    }
    
    if(lockUsersMutex) users_m.unlock();
    return i;
}

sc::User* sc::Context::AddNewUser(sc::User u) {
    users_m.lock();

    sc::User *tmp = NULL;
    std::map<int64_t, User*>::iterator it;

    if(u.id >= 0 && !u.bot) {
        if((it = users.find(u.id)) != users.end()) {
            tmp = it->second;
            tmp->CopyFromUserWhileActive(u);
        } else {
            tmp = new User(u);
            users[u.id] = tmp;
        }
    } else {
        if(u.bot) {
            if(!IsUsernameInUse(u._username)) {
                u.id = FetchNextGenericID(false);
                tmp = new User(u);
                users[u.id] = tmp;
            }
        } else {
            if(!Auth::Reserved(u.__username)) {
                if(GetUserByOriginalName(u._username) == NULL) {
                    u.id = FetchNextGenericID(false);
                    tmp = new User(u);
                    users[u.id] = tmp;
                }
            } else {
                u.id = FetchNextGenericID(false);
                if((tmp = GetUserByOriginalName(u._username)) == NULL) {
                    tmp = new User(u);
                    users[u.id] = tmp;
                } else tmp->CopyFromUserWhileActive(u);
            }
        }
    }
    
    users_m.unlock();
    return tmp;
}

sc::INI sc::Context::Config() {
    return config;
}

uint64_t sc::Context::GetNextMessageId() {
    msgId_m.lock();
    uint64_t tmp = ++msgId;
    msgId_m.unlock();
    return tmp;
}

uint64_t sc::Context::GetCurrentMessageId() {
    msgId_m.lock();
    uint64_t tmp = msgId;
    msgId_m.unlock();
    return tmp;
}

sc::User* sc::Context::GetUserById(int64_t id) {
    sc::User *ret = NULL;
    
    users_m.lock();
    std::map<int64_t, sc::User*>::iterator i;
    if((i = users.find(id)) != users.end())
        ret = i->second;
    users_m.unlock();
    
    return ret;
}

std::vector<sc::User*> sc::Context::GetUsersByFunc(checkFunc f) {
    auto ret = std::vector<sc::User*>();
    
    users_m.lock();
    
    users_m.unlock();
    for(auto i = users.begin(); i != users.end(); ++i) {
        if(f(i->second))
            ret.push_back(i->second);
    }
    return ret;
}

sc::User* sc::Context::GetUserByFunc(checkFunc f) {
    sc::User *u = NULL;
    
    users_m.lock();
    for(auto i = users.begin(); i != users.end(); ++i) {
        if(f(i->second)) {
            u = i->second;
            break;
        }
    }
    users_m.unlock();
    
    return u;
}

sc::User* sc::Context::GetUserByName(std::string name) {
    return GetUserByFunc([&](sc::User *u)->bool {
        return u->GetUsername() == name;
    });
}

sc::User* sc::Context::GetUserByOriginalName(std::string name) {
    return GetUserByFunc([&](sc::User *u)->bool {
        return u->GetOriginalUsername() == name;
    });
}

bool sc::Context::IsUsernameInUse(std::string username) {
    return GetUserByName(username) != NULL;
}

bool sc::Context::IsOriginalUsernameInUse(std::string username) {
    return GetUserByOriginalName(username) != NULL;
}

bool sc::Context::IsUserBanned(User u) {
    bans_m.lock();
    
    bool ret = false, check;
    for(auto i = bans.begin(); i != bans.end(); ++i) {
        if(i->CheckUser(u, &check)) {
            ret = true;
            break;
        } else if(check)
            i = UnlinkBan(i);
    }
    
    bans_m.unlock();
           
    return ret;
}

void sc::Context::CullExpiredBans() {
    bans_m.lock();
    
    for(auto i = bans.begin(); i != bans.end(); ++i) {
        if(i->IsExpired())
            i = UnlinkBan(i);
    }
    
    bans_m.unlock();
}

sc::Channel* sc::Context::GetChannelById(uint32_t id) {
    channelMap_m.lock();
    
    Channel *c = NULL;
    if(channelMapIds.count(id) > 0)
        c = channelMapIds[id];
    
    channelMap_m.unlock();
    return c;
}

sc::Channel* sc::Context::GetChannelByName(std::string name) {
    channelMap_m.lock();

    Channel *ret = NULL;
    if(channelMapNames.count(name) > 0)
        Channel* ret = channelMapNames[name];

    channelMap_m.unlock();
    
    return ret;
}

// ***************** //
// sc::Context::Auth //
// ***************** //

sc::HTTPRequest::Response sc::Context::Auth::RawRequest(sc::imap<> post) {
    return sc::HTTPRequest::Post(
        (std::string)sc::Context::Config()["socket"]["client_root"] + std::string("/?view=auth"), 
        post
    );
}

sc::User sc::Context::Auth::GenerateUserFromString(std::string str) {
    auto parts = sc::str::split(str, "\n");
    if(parts.size() < 4) return sc::User();

    try {
        return sc::User(std::stoull(parts[0]), sc::ip(), parts[1], parts[2], sc::User::Permissions(parts[3]));
    } catch(...) {
        return sc::User();
    }
}

sc::User sc::Context::Auth::Authenticate(std::vector<std::string> args) {
    auto map = sc::imap<>();
    for(int i = 0; i < args.size(); ++i)
        map[std::string("arg") + TOSTR(i + 1)] = args[i];

    auto response = RawRequest(map);

    if(!response.IsValid())
        throw new sc::exception("Authentication server returned garbage data.");
    else if(response.status == 404)
        throw new sc::exception("Authentication server returned 404 not found.");
    else if(response.status != response.OK)
        return sc::User();
    else {
        std::string resp = response.content;
        if(resp.substr(0, 3) == "yes") {
            resp = resp.substr(3);
            return GenerateUserFromString(resp);
        } else return sc::User();
    }
}

bool sc::Context::Auth::Reserved(std::string username) {
    auto response = RawRequest({{"reserve", username}});
    
    if(!response.IsValid())
        throw new sc::exception("Authentication server returned garbage data.");
    else if(response.status == 404)
        throw new sc::exception("Authentication server returned 404 not found.");
    else if(response.status != response.OK)
        return true;
    else
        return response.content == "y";
}

// ********************* //
// sc::Context::Language //
// ********************* //

// ******* //
// sc::Ban //
// ******* //

bool sc::Ban::CheckUser(sc::User u, bool *isExpired) {
    //;; TODO implement
    
    return false;
}

bool sc::Ban::IsExpired() {
    //;; TODO implement
    
    return true;
}

// ********** //
// sc::format //
// ********** //

sc::Message sc::format::usermsg(sc::User u, std::string message, sc::Channel chan,
                                sc::MessageFlags flags, std::string sockstamp, uint64_t msgId) {
    return usermsg(u, message, chan.GetId(), flags, sockstamp, msgId);
}

sc::Message sc::format::usermsg(sc::User u, std::string message, uint32_t chanId,
                                sc::MessageFlags flags, std::string sockstamp, uint64_t msgId) {
    return sc::Message(2, {
        sockstamp == "" ? sc::net::packTime() : sockstamp,
        TOSTR(u.GetId()), u.GetUsername(),
        u.GetColor(), u.GetPermissions(),
        message,
        msgId == 0 ? TOSTR(Context::GetNextMessageId()) : TOSTR(msgId),
        flags.Get(),
        TOSTR(chanId)
    });
}

sc::Message sc::format::usermsg(sc::User u, std::string message, sc::format::SendEnum stype,
                                sc::MessageFlags flags, std::string sockstamp, uint64_t msgId) {
    return sc::Message(2, {
        sockstamp == "" ? sc::net::packTime() : sockstamp,
        TOSTR(u.GetId()), u.GetUsername(),
        u.GetColor(), u.GetPermissions(),
        message,
        msgId == 0 ? TOSTR(Context::GetNextMessageId()) : TOSTR(msgId),
        flags.Get(),
        "0", TOSTR((int)stype)
    });
}

sc::Message sc::format::botmsg(Channel chan, std::string langid,
                               std::vector<std::string> params, bool isError,
                               std::string sockstamp, uint64_t msgId) {
    return botmsg(chan.GetId(), langid, params, isError, sockstamp, msgId);
}

sc::Message sc::format::botmsg(uint32_t chanId, std::string langid,
                               std::vector<std::string> params, bool isError,
                               std::string sockstamp, uint64_t msgId) {
    params.insert(params.begin(), langid);
    params.insert(params.begin(), isError ? "1" : "0");

    return sc::Message(2, {
        sockstamp == "" ? sc::net::packTime() : sockstamp,
        "-1", Context::Config()["structure"]["chatbot_name"],
        "inherit", "",
        sc::str::join(params, "\f"),
        msgId == 0 ? TOSTR(Context::GetNextMessageId()) : TOSTR(msgId),
        MessageFlags(true, true, false, true, true, false, isError, false).Get(),
        TOSTR(chanId)
    });
}

sc::Message sc::format::botmsg(SendEnum stype, std::string langid,
                               std::vector<std::string> params, bool isError,
                               std::string sockstamp, uint64_t msgId) {
    return sc::Message(2, {
        sockstamp == "" ? sc::net::packTime() : sockstamp,
        "-1", Context::Config()["structure"]["chatbot_name"],
        "inherit", "",
        sc::str::join(params, "\f"),
        msgId == 0 ? TOSTR(Context::GetNextMessageId()) : TOSTR(msgId),
        MessageFlags(true, true, false, true, true, false, isError, false).Get(),
        "0", TOSTR((int)stype)
    });
}