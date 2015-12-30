#ifndef SOCKDATABASEH
#define SOCKDATABASEH

#include <vector>
#include <map>
#include "stdcc.h"
#include "sqlite3.h"

namespace sc {
    class LIBPUB Database {
        sqlite3 *conn;
        std::vector<std::string> reservedNames;

        typedef std::map<std::string, sqlite3_stmt*> StmtTable;
        StmtTable preparedStatements;

        // The lock member here determines if those who access this
        // class after it has been constructed have the permission
        // to write additional prepared statements to the existing
        // stack or execute arbitrary SQL via the execute commands.
        // Allowed if false, disallowed if true.
        bool lock;
    public:
        // Initializes a connection to the standard database used
        // by the socklib and core modules, creating the database
        // with the specified schema if it does not exist.
        Database(bool locked = true);

        // Initializes a connection to the database with the name
        // _FILENAME_ located in the working directory. If it did
        // not previously exist, the database is created but the
        // schema is not defined and must be executed in a later
        // query statement.
        Database(std::string filename);

        // Initializes a connection to the database with the name
        // _FILENAME_ located in the working directory. If it did
        // not previously exist, it is created and the _DATABASE
        // SCHEMA_ string is executed to prep the database, as well
        // as sets the reserved statement names in the stmttable.
        Database(std::string filename,
                 std::string databaseSchema,
                 std::vector<std::string> reservedNames = {});

        // Initializes a connection to the database with the name
        // _FILENAME_ located in the working directory. If it did
        // not previously exist, it is created and the _DATABASE
        // SCHEMA_ string is executed to prep the database, as well
        // as sets the reserved statement names in the stmttable.
        // The statement table is preloaded with the defined map,
        // and the database may or may not be locked as defined
        // when the object is constructed.
        Database(std::string filename,
                 std::string databaseSchema,
                 std::map<std::string, std::string> preparedStatements,
                 bool locked = true,
                 std::vector<std::string> reservedNames = {});

        // Prevent copy constructor so the connection isn't deleted
        // or restarted if the object is accidentally copied by value.
        Database(const Database &copy) = delete;

        std::vector<std::string> StatementNames();
        bool IsStatementNameInUse(std::string name);
        bool RegisterStatement(std::string name, std::string query);



        // Automatically clean up the connection and prepared statements
        // after the object has outlived its practical use.
        ~Database();
    };
}

#endif
