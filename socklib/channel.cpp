#include "socklib/context.h"

std::string sc::Channel::SanitizeChannelName(std::string name) {
    //;; TODO this
    return name;
}

sc::Channel::Channel() {
    this->name = "";
    this->backlogs = Backlog((size_t)Context::Config()["Structure"]["backlog_length"]);
}

sc::Channel::Channel(std::string name) {
    this->name = SanitizeChannelName(name);
    this->backlogs = Backlog((size_t)Context::Config()["Structure"]["backlog_length"]);
}

sc::Channel::Channel(std::string name, ModList moderators) {
    this->name = SanitizeChannelName(name);
    this->moderators = moderators;
    this->backlogs = Backlog((size_t)Context::Config()["Structure"]["backlog_length"]);
}

sc::Channel::Channel(sc::Channel &c) {
    this->id = c.id;
    this->name = c.name;
    this->password = c.password;
    this->minRank = c.minRank;
    this->temporary = c.temporary;
    this->users = c.users;
    this->moderators = c.moderators;
    this->backlogs = c.backlogs;
}

uint32_t sc::Channel::GetId() const {
    return this->id;
}

std::string sc::Channel::GetName() const {
    return this->name;
}

std::string sc::Channel::GetPassword() const {
    return this->password;
}

uint16_t sc::Channel::GetMinRank() const {
    return this->minRank;
}

sc::Channel::UserList sc::Channel::Users() const {
    return this->users;
}

sc::Channel::ModList sc::Channel::Moderators() const {
    return this->moderators;
}

sc::Backlog::Logs sc::Channel::Backlogs() const {
    return this->backlogs.Get();
}

bool sc::Channel::Delete() {
    if(this->id == 0) return false;

    Context::UnlinkChannelFromMap(this);
    Context::Broadcast(sc::Message(P_CTX_DEL, {"2", TOSTR(this->id)}));
    
    delete this;
    return true;
}

bool sc::Channel::Delete(User u) {
    return IsChannelOwner(u) || u.GetPermissions().CanModerate()
        ? Delete()
        : false;
}

sc::Channel::JoinError sc::Channel::ForceJoin(User *u, std::string langid) {
    if(!IsInChannel(*u)) {
        Broadcast(sc::Message(P_CTX_DATA, {"0", TOSTR(this->id), "1",
            TOSTR(u->GetId()), u->GetUsername(), u->GetColor(), u->GetPermissions().Get()}));
        Broadcast(sc::format::botmsg(*this, langid, {u->GetUsername()}));
        
        u->Send(sc::Message(P_CHCHAN, {"0", TOSTR(this->id)}));
        
        this->users_m.lock();
        std::vector<std::string> msgargs = {"0", TOSTR(this->id), TOSTR(this->users.size())};
        for(auto i = this->users.begin(); i != this->users.end(); ++i) {
            msgargs.push_back(TOSTR(u->GetId()));
            msgargs.push_back(u->GetUsername());
            msgargs.push_back(u->GetColor());
            msgargs.push_back(u->GetPermissions().Get());
        }
        u->Send(sc::Message(P_CTX_DATA, msgargs));
        
        this->users[u->id] = u;
        this->users_m.unlock();

        u->channels_m.lock();
        u->channels[this->id] = this;
        u->channels_m.unlock();
        
        return OK;
    } else return IN_CHANNEL;
}

sc::Channel::JoinError sc::Channel::Join(User *u, std::string langid, std::string pwdGuess) {
    if(u->GetPermissions().GetRank() >= this->minRank) {
        if(pwdGuess == this->password || this->password == "" || u->GetPermissions().CanModerate()) {
            return ForceJoin(u, langid);
        } else return WRONG_PASSWORD;
    } else return WRONG_RANK;
}

sc::Channel::UserList::iterator sc::Channel::ForceLeave(UserList::iterator it, std::string langid, bool lockChannelsMutex) {
    User *u = it->second;
    auto ret = this->users.erase(it);
    
    u->Send(sc::Message(P_CHCHAN, {"1", TOSTR(this->id)}));
    Broadcast(sc::Message(P_CTX_DEL, {"1", TOSTR(u->GetId()), TOSTR(this->id)}));
    Broadcast(sc::format::botmsg(*this, langid, {u->GetUsername()}));
    
    if(lockChannelsMutex)
        u->channels_m.lock();

    u->channels.erase(this->id);

    if(lockChannelsMutex)
        u->channels_m.unlock();

    return ret;
}

void sc::Channel::Leave(User *u, std::string langid) {
    if(!IsInChannel(*u)) return;
    
    this->users_m.lock();
    this->users.erase(u->GetId());
    this->users_m.unlock();

    u->Send(sc::Message(P_CHCHAN, {"1", TOSTR(this->id)}));
    Broadcast(sc::Message(P_CTX_DEL, {"1", TOSTR(u->GetId()), TOSTR(this->id)}));
    Broadcast(sc::format::botmsg(*this, langid, {u->GetUsername()}));

    u->channels_m.lock();
    u->channels.erase(this->id);
    u->channels_m.unlock();
}

bool sc::Channel::IsChannelCreator(sc::User u) {
    return User::TestPair(u, this->creator);
}

bool sc::Channel::IsChannelOwner(sc::User u) {
    this->moderators_m.lock();
    
    for(auto i = this->moderators.begin(); i != this->moderators.end(); ++i) {
        if(User::TestPair(u, std::make_pair(std::get<0>(*i), std::get<1>(*i))) && std::get<2>(*i) == true) {
            this->moderators_m.unlock();
            return true;
        }
    }
    
    this->moderators_m.unlock();
    return false;
}

bool sc::Channel::IsChannelModerator(sc::User u) {
    this->moderators_m.lock();
    
    for(auto i = this->moderators.begin(); i != this->moderators.end(); ++i) {
        if(User::TestPair(u, std::make_pair(std::get<0>(*i), std::get<1>(*i))) && std::get<2>(*i) == false) {
            this->moderators_m.unlock();
            return true;
        }
    }
    
    this->moderators_m.unlock();
    return false;
}

bool sc::Channel::CanModerateChannel(sc::User u) {
    if(IsChannelCreator(u)) return true;
    
    this->moderators_m.lock();
    
    for(auto i = this->moderators.begin(); i != this->moderators.end(); ++i) {
        if(User::TestPair(u, std::make_pair(std::get<0>(*i), std::get<1>(*i)))) {
            this->moderators_m.unlock();
            return true;
        }
    }
    
    this->moderators_m.unlock();
    return false;
}

bool sc::Channel::IsInChannel(sc::User u) {
    return this->users.find(u.GetId()) != this->users.end();
}

void sc::Channel::Promote(sc::User u, sc::Channel::PromotionType type) {
    auto test = [&] (std::tuple<int64_t, std::string, bool> t) -> bool {
        return sc::User::TestPair(u, std::make_pair(std::get<0>(t), std::get<1>(t)));
    };
    
    if(type == NORMAL) {
        this->moderators_m.lock();
        
        auto i = std::find_if(this->moderators.begin(), this->moderators.end(), test);
        if(i != this->moderators.end())
            this->moderators.erase(i);
        
        this->moderators_m.unlock();
    } else {
        this->moderators_m.lock();
        
        auto i = std::find_if(this->moderators.begin(), this->moderators.end(), test);
        if(i == this->moderators.end())
            this->moderators.push_back(std::make_tuple(u.GetId(), u.GetUsername(), type == MODERATOR ? false : true));
        
        this->moderators_m.unlock();
    }
}

bool sc::Channel::Promote(sc::User u, sc::Channel::PromotionType type, sc::User requestingUser) {
    if(IsChannelOwner(requestingUser) || IsChannelCreator(requestingUser) || requestingUser.GetPermissions().CanModerate()) {
        Promote(u, type);
        return true;
    } else return false;
}

std::vector<std::string> sc::Channel::GetParameters() {
    return {
        "1", "1",
        TOSTR(this->id),
        this->name,
        this->password == "" ? "0" : "1",
        this->temporary ? "1" : "0"
    };
}

void sc::Channel::Rename(std::string name) {
    if(this->id == 0) return;
    
    name_m.lock();
    this->name = SanitizeChannelName(name);
    // TODO update channel map
    Context::Broadcast(sc::Message(P_CTX_DATA, GetParameters()));
    name_m.unlock();
}

bool sc::Channel::Rename(std::string name, sc::User u) {
    if(this->id != 0 && CanModerateChannel(u)) {
        Rename(name);
        return true;
    } else return false;
}

void sc::Channel::SetMinRank(uint16_t rank) {
    if(this->id == 0) return;
    
    minRank_m.lock();
    this->minRank = rank;
    Context::Broadcast(sc::Message(P_CTX_DATA, GetParameters()));
    
    users_m.lock();
    for(auto i = this->users.begin(); i != this->users.end(); ++i) {
        if(i->second->GetPermissions().GetRank() < rank)
            i = ForceLeave(i, "lchrankerr"); //;; TODO add lchrankerr to lang files
    }
    users_m.unlock();
    minRank_m.unlock();
}

bool sc::Channel::SetMinRank(uint16_t rank, User u) {
    if(this->id != 0 && CanModerateChannel(u) && rank <= u.GetPermissions().GetRank()) {
        SetMinRank(rank);
        return true;
    } else return false;
}

void sc::Channel::ResetPassword() {
    password_m.lock();
    this->password = "";
    Context::Broadcast(sc::Message(P_CTX_DATA, GetParameters()));
    password_m.unlock();
}

bool sc::Channel::ResetPassword(sc::User u) {
    if(IsChannelOwner(u) || IsChannelCreator(u)) {
        ResetPassword();
        return true;
    } else return false;
}

void sc::Channel::SetPassword(std::string pwd) {
    password_m.lock();
    this->password = pwd;
    Context::Broadcast(sc::Message(P_CTX_DATA, GetParameters()));
    password_m.unlock();
}

bool sc::Channel::SetPassword(std::string pwd, sc::User u) {
    if(IsChannelOwner(u) || IsChannelCreator(u)) {
        SetPassword(pwd);
        return true;
    } else return false;
}

void sc::Channel::Broadcast(sc::Message msg) {
    backlogs_m.lock();
    users_m.lock();
    
    backlogs.Push(msg);
    for(auto i = this->users.begin(); i != this->users.end(); ++i)
        i->second->Send(msg);

    users_m.unlock();
    backlogs_m.unlock();
}