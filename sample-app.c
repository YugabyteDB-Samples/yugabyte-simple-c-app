#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpq-fe.h"

#define HOST ""
#define PORT "5433"
#define DB_NAME "yugabyte"
#define USER ""
#define PASSWORD ""
#define SSL_MODE "verify-full"
#define SSL_ROOT_CERT ""

#define CONN_STR "host=" HOST " port=" PORT " dbname=" DB_NAME \
                 " user=" USER " password=" PASSWORD \
                 " sslmode=" SSL_MODE " sslrootcert=" SSL_ROOT_CERT

PGconn* connect();
void createDatabase(PGconn *conn);
void selectAccounts(PGconn *conn);
void printErrorAndExit(PGconn *conn, PGresult *res);
void transferMoneyBetweenAccounts(PGconn *conn);

int main(int argc, char **argv) {
    PGconn  *conn;

    conn = connect();
    createDatabase(conn);
    selectAccounts(conn);
    transferMoneyBetweenAccounts(conn);
    selectAccounts(conn);
    
    PQfinish(conn);

    return 0;
}

PGconn* connect() {
    PGconn *conn;

    printf(">>>> Connecting to YugabyteDB!\n");

    PQinitSSL(1);
    
    conn = PQconnectdb(CONN_STR);

    if (PQstatus(conn) != CONNECTION_OK) {
        printErrorAndExit(conn, NULL);
    }

    printf(">>>> Successfully connected to YugabyteDB!\n");

    return conn;
}

void createDatabase(PGconn *conn) {
    PGresult *res;

    res = PQexec(conn, "DROP TABLE IF EXISTS DemoAccount");

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    res = PQexec(conn, "CREATE TABLE DemoAccount ( \
                        id int PRIMARY KEY, \
                        name varchar, \
                        age int, \
                        country varchar, \
                        balance int)");
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    res = PQexec(conn, "INSERT INTO DemoAccount VALUES \
                        (1, 'Jessica', 28, 'USA', 10000), \
                        (2, 'John', 28, 'Canada', 9000)");

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    PQclear(res);

    printf(">>>> Successfully created table DemoAccount.\n");
}

void selectAccounts(PGconn *conn) {
    PGresult *res;
    int i;

    printf(">>>> Selecting accounts:\n");

    res = PQexec(conn, "SELECT name, age, country, balance FROM DemoAccount");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printErrorAndExit(conn, res);
    }
    
    for (i = 0; i < PQntuples(res); i++) {
      printf("name = %s, age = %s, country = %s, balance = %s\n", 
          PQgetvalue(res, i, 0), PQgetvalue(res, i, 1), PQgetvalue(res, i, 2), PQgetvalue(res, i, 3));
    }

    PQclear(res);
}

void transferMoneyBetweenAccounts(PGconn *conn) {
    PGresult *res;

    res = PQexec(conn, "BEGIN TRANSACTION");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    res = PQexec(conn, "UPDATE DemoAccount SET balance = balance - 800 WHERE name = \'Jessica\'");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    res = PQexec(conn, "UPDATE DemoAccount SET balance = balance + 800 WHERE name = \'John\'");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printErrorAndExit(conn, res);
    }

    PQclear(res);

    printf(">>>> Transferred 800 between accounts\n");
}

void printErrorAndExit(PGconn *conn, PGresult *res) {
    char *errCode;

    if (res) {
        errCode = PQresultErrorField(res, PG_DIAG_SQLSTATE);

        // Applies to logic of the transferMoneyBetweenAccounts method
        if (errCode && strcmp(errCode, "40001")) {
            printf("The operation is aborted due to a concurrent transaction that is modifying the same set of rows. \
                    Consider adding retry logic for production-grade applications.\n");
        }

        PQclear(res);
    }

    fprintf(stderr, "%s", PQerrorMessage(conn));

    PQfinish(conn);
    exit(1);
}

