-- DO NOT FORGET TO “PRAGMA foreign_keys = ON;” WHEN SPAWNING A CONNECTION !!

BEGIN TRANSACTION;

-- Table: channels
CREATE TABLE channels (
    id    INTEGER NOT NULL,
    pid   INTEGER DEFAULT NULL,
    name  TEXT    NOT NULL,
    pwd   TEXT    DEFAULT NULL,
    rank  INTEGER NOT NULL
                  DEFAULT 0
                  CHECK (rank >= 0),
    tmp   INTEGER NOT NULL
                  DEFAULT 0
                  CHECK (tmp BETWEEN 0 AND 1),
    cperm INTEGER NOT NULL
                  DEFAULT 0
                  CHECK (cperm BETWEEN 0 AND 2),
    PRIMARY KEY (
        id
    ),
    FOREIGN KEY (
        pid
    )
    REFERENCES channels (id) ON DELETE CASCADE
)
WITHOUT ROWID;

-- Table: channel_mods
CREATE TABLE channel_mods (
    chid INTEGER REFERENCES channels (id) ON DELETE CASCADE
                 NOT NULL,
    perm INTEGER CHECK (perm BETWEEN 0 AND 2) 
                 NOT NULL
                 DEFAULT (0),
    id   INTEGER NOT NULL,
    name TEXT    NOT NULL
);

COMMIT TRANSACTION;
