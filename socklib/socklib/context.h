#ifndef SOCKCTXH
#define SOCKCTXH

#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <stdint.h>
#include <mutex>
#include "socket.h"
#include "msg.h"
#include "ini.h"
#include "utils.h"

namespace sc {
    class Channel;
    class Context;
    class User;

    class LIBPUB Ban {
        std::tuple<int64_t, std::string, bool> user;
        std::pair<sc::ip, bool> ip;
        std::pair<std::chrono::time_point<std::chrono::steady_clock>, bool> expire;
        uint32_t rankAdministered;
    public:
        enum LIBPUB BanType { USERBAN, IPBAN };

        Ban();
        Ban(User u, uint32_t rank, BanType type = USERBAN);
        // duration is in minutes
        Ban(User u, uint64_t duration, uint32_t rank, BanType type = USERBAN);
        Ban(sc::ip ip, uint64_t duration, uint32_t rank);
        Ban(std::tuple<int64_t, std::string, bool> user, std::pair<sc::ip, bool> ip,
            std::pair<std::chrono::time_point<std::chrono::steady_clock>, bool> expire,
            uint32_t rank);

        bool CheckUser(User u, bool *isExpired = NULL);
        bool IsExpired();
        uint32_t GetRankAdministered();
    };

    struct LIBPUB MessageFlags {
        bool flags[8];

        MessageFlags() : MessageFlags(true, false, false, true, true, true, false, false) {}

        MessageFlags(bool boldName, bool italicName, bool underlineName, bool colon, 
                     bool alert, bool sanitize, bool isError, bool isBacklog) {
            flags[0] = boldName; flags[1] = italicName;
            flags[2] = underlineName; flags[3] = colon;
            flags[4] = alert; flags[5] = sanitize;
            flags[6] = isError; flags[7] = isBacklog;
        }

        std::string Get() const {
            std::stringstream ss;
            for(int i = 0; i < 7; i++)
                ss << (flags[i] ? "1" : "0");
            return ss.str();
        }
    };

    class LIBPUB User {
    public:
        class LIBPUB Permissions {
            uint16_t rank;
            uint8_t stdperms[4];
            std::map<std::string, uint8_t> custom;

            void Error();
            bool Bound(uint8_t check, uint8_t lower, uint8_t upper) const;
        public:
            enum LIBPUB UserType           { ILLEGAL_UT = -1, NORMAL, MODERATOR, ADMIN };
            enum LIBPUB CreateChannelType  { ILLEGAL_CCT = -1, DISABLED, TEMPORARY, PERMANENT };

            Permissions();
            Permissions(std::string permstr);
            Permissions(uint16_t rank, uint8_t stdperms[4], std::map<std::string, uint8_t> custom);

            uint16_t GetRank() const;
            void SetRank(uint16_t rank);

            bool CanModerate() const;
            UserType GetUserType() const;
            void SetUserType(UserType type);

            bool CanViewLogs() const;
            void CanViewLogs(bool can);

            bool CanChangeUsername() const;
            void CanChangeUsername(bool can);

            CreateChannelType GetCreateChannelType() const;
            void SetCreateChannelType(CreateChannelType type);

            bool CheckCustomPermission(std::string name) const;
            uint8_t GetCustomPermission(std::string name);
            void SetCustomPermission(std::string name, uint8_t value);

            std::string Get();
            operator std::string();
        };

        User();
        User(User &u);

        // constructor for a legitimate user
        User(int64_t id, sc::ip ip, std::string username, std::string color,
             Permissions permissions = Permissions());

        // constructor for a bot
        User(std::string username, std::string color);
        
        void CopyFromUserWhileActive(User u);

        int64_t GetId() const;
        std::vector<sc::ip> GetIpList() const;
        bool CheckIp(sc::ip ip);
        bool IsBot() const;
        bool IsValid() const;

        std::string GetUsername() const;
        std::string GetOriginalUsername() const;
        std::string GetOriginalUnsanitizedUsername() const;

        void SetUsername(std::string username);
        void ResetUsername();

        bool IsInChannel(uint32_t channelId) const;
        bool IsInChannel(std::string channelName);

        std::string GetColor() const;
        std::string GetOriginalColor() const;

        void SetColor(std::string color);
        void ResetColor();

        Permissions GetPermissions() const;
        Permissions GetOriginalPermissions() const;

        void SetPermissions(Permissions perms);
        void ResetPermissions();

        void Kick();

        void Send(sc::Message msg);

        class LIBPUB Connection {
            typedef std::queue<sc::Message> ActionQueue;
            ActionQueue queue;
            std::mutex queue_m;
            bool closed;
            std::mutex closed_m;

            sc::ip ip;
        public:
            Connection(sc::ip ip = sc::ip("::"));
            Connection(const Connection &c);

            Connection& operator= (const Connection &c);

            void PushMessage(sc::Message msg);

            sc::Message PopMessage();
            bool CanPopMessage();

            void Close();
            bool IsClosed();

            sc::ip GetIP();
        };

        void HookConnection(Connection *conn);
        void UnhookConnection(Connection *conn);

        static bool TestPair(User u, std::pair<int64_t, std::string> p);
        static std::string SanitizeUsername(std::string name);

        friend Channel;
        friend Context;
    private:
        void CommitChange();

        int64_t id;
        bool bot;

        std::string username; // clean username
        std::string _username; // original clean username
        
        //;; TODO consider not sanitizing usernames at all to prevent mess
        std::string __username; // unsanitized version of original username
        
        std::mutex username_m;

        std::string color; // user color
        std::string _color; // original user color
        std::mutex color_m;

        Permissions permissions; // user permissions
        Permissions _permissions; // original user permissions
        std::mutex permissions_m;

        std::vector<std::string> args;
        
        std::map<uint32_t, Channel*> channels = std::map<uint32_t, Channel*>();
        std::mutex channels_m;

        typedef std::vector<Connection *> ConnStack;
        ConnStack conns = ConnStack();
        std::mutex conns_m;
    };

    class LIBPUB Channel {
    public:
        typedef std::map<int64_t, User*> UserList;
        typedef std::vector<std::tuple<int64_t, std::string, bool>> ModList;
    private:
        std::pair<int64_t, std::string> creator = std::make_pair(-1, "");

        uint32_t id = 0;

        std::string name;
        std::mutex name_m;

        std::string password = "";
        std::mutex password_m;

        uint16_t minRank = 0;
        std::mutex minRank_m;

        bool temporary = false;
        std::mutex temporary_m;
        
        UserList users = UserList();
        std::mutex users_m;

        ModList moderators = ModList();
        std::mutex moderators_m;

        Backlog backlogs;
        std::mutex backlogs_m;
        
        Channel();
        Channel(std::string name);
        Channel(std::string name, ModList moderators);
        
        LIBPRIV std::vector<std::string> GetParameters();
        LIBPRIV UserList::iterator ForceLeave(UserList::iterator u, std::string langid = "lchan", bool lockChannelsMutex = true);
    public:
        Channel(Channel &c);
        
        uint32_t GetId() const;
        std::string GetName() const;
        std::string GetPassword() const;
        uint16_t GetMinRank() const;

        UserList Users() const;
        ModList Moderators() const;
        Backlog::Logs Backlogs() const;

        // deletes without checking user permissions
        bool Delete();
        // only deletes if user fulfills required permissions
        bool Delete(User u);

        enum LIBPUB JoinError { OK, WRONG_PASSWORD, WRONG_RANK, IN_CHANNEL, ILLEGAL_CHANNEL };
        JoinError Join(User *u, std::string langid = "jchan", std::string pwdGuess = "");
        JoinError ForceJoin(User *u, std::string langid = "jchan");

        void Leave(User *u, std::string langid = "lchan");

        bool IsChannelCreator(User u);
        bool IsChannelOwner(User u);
        bool IsChannelModerator(User u);
        bool CanModerateChannel(User u);
        
        bool IsInChannel(User u);

        enum LIBPUB PromotionType { NORMAL, MODERATOR, OWNER };
        // promotes user without checking if requesting user has ability to
        void Promote(User u, PromotionType type);
        // promotes user only if requesting user has permission to do so
        bool Promote(User u, PromotionType type, User requestingUser);

        void Rename(std::string name);
        bool Rename(std::string name, User u);

        void SetMinRank(uint16_t rank);
        bool SetMinRank(uint16_t rank, User u);
        
        void ResetPassword();
        bool ResetPassword(sc::User u);
        
        void SetPassword(std::string pwd);
        bool SetPassword(std::string pwd, sc::User u);

        void Broadcast(sc::Message msg);
        
        static std::string SanitizeChannelName(std::string name);

        friend Context;
    };

    class LIBPUB Context {
        static sc::INI config;

        static uint64_t msgId;
        static std::mutex msgId_m;

        static std::vector<Ban>::iterator UnlinkBan(std::vector<Ban>::iterator i);
        static std::vector<Ban> bans;
        static std::mutex bans_m;

        static void UnlinkUserFromMap(User *u, bool lockUsersMutex = true);
        static int64_t FetchNextGenericID(bool lockUsersMutex = true);
        static std::map<int64_t, User*> users;
        static std::mutex users_m;

        static void UnlinkChannelFromMap(Channel *c);
        static void AddChannelToMap(Channel *c);
        static std::map<uint32_t, Channel*> channelMapIds;
        static sc::imap<Channel*> channelMapNames;
        static std::mutex channelMap_m;
    public:
        class LIBPUB Auth {
            static sc::HTTPRequest::Response RawRequest(sc::imap<> post);
            static User GenerateUserFromString(std::string str);
        public:
            static User Authenticate(std::vector<std::string> args);
            static User Validate(int64_t id, std::string username);
            static bool Reserved(std::string username);
        };
        
        class LIBPUB Language {
            // 0 - Bot Text
            // 1 - Bot Error Text
            // 2 - Manual Pages
            typedef std::map<std::string, std::map<std::string, std::string>[3]> LangMap;
            
            static LangMap lmap;
            static std::mutex lmap_m;
            void Init();
        public:
            std::string ResolveBotMessage(std::string lang, std::string langid, std::vector<std::string> params = {}, bool isError = false);
            std::string ResolveManualMessage(std::string lang, std::string cmd);
        };
        
        static void Init();

        static void Broadcast(sc::Message msg);

        static User* AddNewUser(User u);
        
        static sc::INI Config();

        static Channel* CreateChannel(std::string name, std::string password = "", uint16_t minRank = 0, bool temporary = false, User *creator = NULL);

        static uint64_t GetNextMessageId();
        static uint64_t GetCurrentMessageId();

        typedef std::function<bool(User*)> checkFunc;
        static std::vector<User*> GetUsersByFunc(checkFunc f);
        static User* GetUserByFunc(checkFunc f);
        
        static User* GetUserById(int64_t id);
        static User* GetUserByName(std::string name);
        static User* GetUserByOriginalName(std::string name);
        //;; TODO consider getting rid of below function
        //static User* GetUserByUnsanitizedName(std::string name);

        static bool IsUsernameInUse(std::string username);
        static bool IsOriginalUsernameInUse(std::string username);
        static bool IsUserBanned(User u);
        static void CullExpiredBans();

        static Channel* GetChannelById(uint32_t id);
        static Channel* GetChannelByName(std::string name);

        friend Ban;
        friend User;
        friend Channel;
    };
    
    
    struct LIBPUB format {
        static sc::Message
            usermsg(User u, std::string message, Channel chan,
                    sc::MessageFlags flags = sc::MessageFlags(),
                    std::string sockstamp = "", uint64_t msgId = 0);
        static sc::Message
            botmsg(Channel chan, std::string langid,
                   std::vector<std::string> params, bool isError = false,
                   std::string sockstamp = "", uint64_t msgId = 0);

        static sc::Message
            usermsg(User u, std::string message, uint32_t chanId,
                    sc::MessageFlags flags = sc::MessageFlags(),
                    std::string sockstamp = "", uint64_t msgId = 0);

        static sc::Message
            botmsg(uint32_t chanId, std::string langid,
                   std::vector<std::string> params, bool isError = false,
                   std::string sockstamp = "", uint64_t msgId = 0);
        
        enum LIBPUB SendEnum { ALL = 1, LOCAL, PRIVBY, PRIVTO };
        static sc::Message
            usermsg(User u, std::string message, SendEnum stype,
                    sc::MessageFlags flags = sc::MessageFlags(),
                    std::string sockstamp = "", uint64_t msgId = 0);
        static sc::Message
            botmsg(SendEnum stype, std::string langid,
                   std::vector<std::string> params = {}, bool isError = false,
                   std::string sockstamp = "", uint64_t msgId = 0);
    };
}

#endif