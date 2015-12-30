#include "socklib/context.h"

#pragma region Connection

sc::User::Connection::Connection(sc::ip ip) {
    this->queue = ActionQueue();
    this->closed = false;
    this->ip = ip;
}

sc::User::Connection::Connection(const Connection &c) {
    this->queue = c.queue;
    this->closed = c.closed;
    this->ip = c.ip;
}

sc::User::Connection& sc::User::Connection::operator= (const Connection &c) {
    this->queue = c.queue;
    this->closed = c.closed;
    this->ip = c.ip;

    return *this;
}

void sc::User::Connection::PushMessage(sc::Message msg) {
    queue_m.lock();
    this->queue.push(msg);
    queue_m.unlock();
}

sc::Message sc::User::Connection::PopMessage() {
    queue_m.lock();
    sc::Message ret;
    if(CanPopMessage()) {
        ret = this->queue.front();
        this->queue.pop();
    } else
        ret = sc::Message();
    queue_m.unlock();
    return ret;
}

bool sc::User::Connection::CanPopMessage() {
    return this->queue.size() > 0;
}

void sc::User::Connection::Close() {
    this->closed_m.lock();
    this->closed = true;
    this->closed_m.unlock();
}

bool sc::User::Connection::IsClosed() {
    return this->closed;
}

sc::ip sc::User::Connection::GetIP() {
    return this->ip;
}

#pragma endregion

#pragma region Permissions

void sc::User::Permissions::Error() {
    this->rank = 0xFFFF;
    for(int i = 0; i < 4; i++)
        this->stdperms[i] = 0xFF;

    this->custom = std::map<std::string, uint8_t>();
}

sc::User::Permissions::Permissions() {
    Error();
}

sc::User::Permissions::Permissions(std::string permstr) {
    auto perms = str::split(permstr, ",");
    if(perms.size() < 5) {
        Error();
        return;
    }

    this->rank = std::stoi(perms[0]);
    for(int i = 1; i < 5; i++)
        this->stdperms[i - 1] = std::stoi(perms[i]);

    this->custom = std::map<std::string, uint8_t>();
    for(int i = 5; i < perms.size(); i++) {
        auto cperm = str::split(perms[i], "=", 2);
        if(cperm.size() != 2)
            continue;
        this->custom[cperm[0]] = std::stoi(cperm[1]);
    }
}

sc::User::Permissions::Permissions(uint16_t rank, uint8_t stdperms[4],
                                   std::map<std::string, uint8_t> custom) {
    this->rank = rank;
    for(int i = 0; i < 4; i++)
        this->stdperms[i] = stdperms[i];
    this->custom = custom;
}

uint16_t sc::User::Permissions::GetRank() const {
    return this->rank;
}

void sc::User::Permissions::SetRank(uint16_t rank) {
    this->rank = rank;
}

bool sc::User::Permissions::Bound(uint8_t check, uint8_t lower, uint8_t upper) const {
    return check >= lower && check <= upper;
}

bool sc::User::Permissions::CanModerate() const {
    return GetUserType() == UserType::MODERATOR || GetUserType() == UserType::ADMIN;
}

sc::User::Permissions::UserType sc::User::Permissions::GetUserType() const {
    return Bound(this->stdperms[0], 0, 2) ? static_cast<UserType>(this->stdperms[0])
                                          : UserType::ILLEGAL_UT;
}

void sc::User::Permissions::SetUserType(sc::User::Permissions::UserType type) {
    this->stdperms[0] = type;
}

bool sc::User::Permissions::CanViewLogs() const {
    return !(this->stdperms[1] == 0);
}

void sc::User::Permissions::CanViewLogs(bool can) {
    this->stdperms[1] = can ? 1 : 0;
}

bool sc::User::Permissions::CanChangeUsername() const {
    return !(this->stdperms[2] == 0);
}

sc::User::Permissions::CreateChannelType sc::User::Permissions::GetCreateChannelType() const {
    return Bound(this->stdperms[3], 0, 2) ? static_cast<CreateChannelType>(this->stdperms[1])
                                          : CreateChannelType::ILLEGAL_CCT;
}

void sc::User::Permissions::SetCreateChannelType(sc::User::Permissions::CreateChannelType type) {
    this->stdperms[3] = type;
}

bool sc::User::Permissions::CheckCustomPermission(std::string name) const {
    return this->custom.count(name) > 0;
}

uint8_t sc::User::Permissions::GetCustomPermission(std::string name) {
    if(CheckCustomPermission(name)) return this->custom[name];
    else return 0xFF;
}

void sc::User::Permissions::SetCustomPermission(std::string name, uint8_t value) {
    this->custom[name] = value;
}

std::string sc::User::Permissions::Get() {
    std::stringstream ss;
    for(int i = 0; i < 4; i++)
        ss << (i != 0 ? "," : "") << this->stdperms[i];

    if(this->custom.size() > 0) {
        for(auto i = this->custom.begin(); i != this->custom.end(); ++i)
            ss << "," << i->first << "=" << i->second;
    }

    return ss.str();
}

sc::User::Permissions::operator std::string() {
    return Get();
}

#pragma endregion

bool sc::User::TestPair(sc::User u, std::pair<int64_t, std::string> p) {
    if(p.first == -1) return false;
    else if(p.first >= 0 && p.first == u.GetId()) return true;
    else if(p.first < 0 && p.second == u.GetUsername()) return true;
    else return false;
}

std::string sc::User::SanitizeUsername(std::string name) {
    //;; TODO this
    return name;
}

sc::User::User() : id(-1), bot(false) {
}

sc::User::User(int64_t id, sc::ip ip, std::string username,
               std::string color, Permissions permissions) {
    this->bot = false;

    this->id = id;
    this->__username = username;

    this->username = this->_username = SanitizeUsername(username);
    this->color = this->_color = color;
    this->permissions = this->_permissions = permissions;

    this->args = std::vector<std::string>();
}

sc::User::User(std::string username, std::string color) {
    this->bot = true;
    
    this->id = -2;
    this->__username = username;

    this->username = this->_username = SanitizeUsername(username);
    this->color = this->_color = color;
    this->permissions = Permissions();

    this->args = std::vector<std::string>();
}

sc::User::User(sc::User &u) {
    this->id = u.id;
    this->bot = u.bot;
    this->username = u.username;
    this->_username = u._username;
    this->__username = u.__username;
    this->color = u.color;
    this->_color = u._color;
    this->permissions = u.permissions;
    this->_permissions = u._permissions;
    this->args = u.args;
    this->channels = u.channels;
    this->conns = u.conns;
}

void sc::User::CopyFromUserWhileActive(sc::User u) {
    this->username_m.lock();
    this->username = this->username == this->_username ? u.username : this->username;
    this->_username = u._username;
    this->__username = u.__username;
    this->username_m.unlock();
    
    this->color_m.lock();
    this->color = this->color == this->_color ? u.color : this->color;
    this->_color = u._color;
    this->color_m.unlock();
    
    this->permissions_m.lock();
    this->permissions = this->permissions.Get() == this->_permissions.Get() ? u.permissions : this->permissions;
    this->_permissions = u._permissions;
    this->permissions_m.unlock();
}

int64_t sc::User::GetId() const {
    return this->id;
}

std::vector<sc::ip> sc::User::GetIpList() const {
    std::vector<sc::ip> ret = std::vector<sc::ip>();

    /*for(auto i = this->conns.begin(); i != this->conns.end(); ++i) {
        if((*i)->GetIP() != sc::ip("::") && std::find(this->conns.begin(), this->conns.end(), (*i)->GetIP()) == this->conns.end())
            ret.push_back((*i)->GetIP());
    }*/

    return ret;
}

bool sc::User::IsBot() const {
    return this->bot;
}

bool sc::User::IsValid() const {
    return this->id != -1;
}

bool sc::User::CheckIp(sc::ip ip) {
    conns_m.lock();

    bool ret = false;
    /*for(auto i = conns.begin(); i != conns.end(); ++i) {
        if((*i)->GetIP() == ip && (*i)->GetIP() != sc::ip("::")) {
            ret = true;
            break;
        }
    }*/

    conns_m.unlock();

    return ret;
}

std::string sc::User::GetUsername() const {
    return this->username;
}

std::string sc::User::GetOriginalUsername() const {
    return this->_username;
}

std::string sc::User::GetOriginalUnsanitizedUsername() const {
    return this->__username;
}

void sc::User::SetUsername(std::string username) {
    username_m.lock();
    this->username = SanitizeUsername(username);
    username_m.unlock();
    CommitChange();
}

void sc::User::ResetUsername() {
    username_m.lock();
    this->username = this->_username;
    username_m.unlock();
    CommitChange();
}

bool sc::User::IsInChannel(uint32_t channelId) const {
    return channels.count(channelId) > 0;
}

bool sc::User::IsInChannel(std::string channelName) {
    channels_m.lock();
    bool ret = false;
    for(auto i = channels.begin(); i != channels.end(); ++i) {
        if(sc::str::tolower(i->second->GetName()) == sc::str::tolower(channelName)) {
            ret = true;
            break;
        }
    }
    channels_m.unlock();
    return ret;
}

std::string sc::User::GetColor() const {
    return this->color;
}

std::string sc::User::GetOriginalColor() const {
    return this->_color;
}

void sc::User::SetColor(std::string color) {
    color_m.lock();
    this->color = color;
    color_m.unlock();
    CommitChange();
}

void sc::User::ResetColor() {
    color_m.lock();
    this->color = this->_color;
    color_m.unlock();
    CommitChange();
}

sc::User::Permissions sc::User::GetPermissions() const {
    return this->permissions;
}

sc::User::Permissions sc::User::GetOriginalPermissions() const {
    return this->_permissions;
}

void sc::User::SetPermissions(sc::User::Permissions perms) {
    permissions_m.lock();
    this->permissions = perms;
    permissions_m.unlock();
    CommitChange();
}

void sc::User::ResetPermissions() {
    permissions_m.lock();
    this->permissions = this->_permissions;
    permissions_m.unlock();
    CommitChange();
}

void sc::User::Kick() {
    conns_m.lock();
    for(auto i = conns.begin(); i != conns.end(); ++i)
        (*i)->Close();
    conns_m.unlock();
}

void sc::User::Send(sc::Message msg) {
    conns_m.lock();
    for(auto i = conns.begin(); i != conns.end(); ++i)
        (*i)->PushMessage(msg);
    conns_m.unlock();
}

void sc::User::HookConnection(sc::User::Connection *conn) {
    conns_m.lock();
    if(std::find(conns.begin(), conns.end(), conn) == conns.end())
        conns.push_back(conn);
    conns_m.unlock();
}

void sc::User::UnhookConnection(sc::User::Connection *conn) {
    Context::users_m.lock();
    conns_m.lock();
    auto end = std::remove(conns.begin(), conns.end(), conn);
    conns.erase(end, conns.end());
    conns_m.unlock();

    if(conns.size() == 0) {
        sc::Context::UnlinkUserFromMap(this, false);
        Context::users_m.unlock();
        delete this;
    } else
        Context::users_m.unlock();
}

void sc::User::CommitChange() {
    if(this->id != -1 && this->id != -2) {
        Context::Broadcast(sc::Message(P_CTX_DATA, {
            "0", "0", "1",
            TOSTR(this->id),
            this->username,
            this->color,
            this->permissions.Get()
        }));
    }
}