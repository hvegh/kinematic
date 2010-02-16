
#include "SqliteLogger.h"


SqliteLogger::SqliteLogger(const char* filename, RawReceiver& gps, int station_id)
                 : filename(filename), gps(gps), station_id(station_id)
{
    debug("SqliteLogger::SqliteLogger(%s)\n", filename);

    // Open the database
    ErrCode = Initialize();
    if (ErrCode != OK) Cleanup();
}


bool SqliteLogger::OutputEpoch()
{
    debug("SqliteLogger::OutputEpoch\n");
    // for each valid observation
    for (int s=0; s<MaxSats; s++) {
        if (!gps.obs[s].Valid) continue;

        // Insert observation into the database
        sqlite3_bind_int(insert, 1, station_id);
        sqlite3_bind_int64(insert, 2, (sqlite3_int64)gps.GpsTime);
        sqlite3_bind_int(insert, 3, SatToSvid(s));
        sqlite3_bind_double(insert, 4, gps.obs[s].PR); 
        sqlite3_bind_double(insert, 5, gps.obs[s].Phase);
        sqlite3_bind_double(insert, 6, gps.obs[s].Doppler);
        sqlite3_bind_double(insert, 7, gps.obs[s].SNR);
        sqlite3_bind_int(insert, 8, gps.obs[s].Slip);

        debug("About to insert row: svid=%d\n", SatToSvid(s));
        int rc = sqlite3_step(insert);
        if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
            Error("Can't insert observation: %s\n", sqlite3_errmsg(db)); 
        if (sqlite3_reset(insert) != SQLITE_OK)
            return Error("Can't cleanup for next insert:%s\n", sqlite3_errmsg(db));
    }

    return OK;
}



SqliteLogger::~SqliteLogger()
{
    Cleanup();
}



bool SqliteLogger::Initialize()
{
 
    // Open the database
    if (sqlite3_open(filename, &db) != SQLITE_OK)
        return Error("Can't open database at %s: %s\n", filename, sqlite3_errmsg(db));

    // Create the observation table if not already done
    const char* sql;
    sql = "create table if not exists observation "
                  " (station_id int16, " 
                  "  time       int64, "
                  "  svid       int8, "
                  "  PR         double, "
                  "  phase      double, "
                  "  doppler    double, "
                  "  snr        double, "
                  "  slipped    boolean);";                  

    // TODO: Add an index on station_id, time, svid

    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
        return Error("Sqlite logger %s can't create the observation tables: %s\n", 
                       filename, sqlite3_errmsg(db));

    // Prepare an insert statement
    sql = "insert into observation "
                "(station_id, time, svid, PR, phase, doppler,snr, slipped) "
                "values (?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, strlen(sql), &insert, NULL) != SQLITE_OK)
        return Error("Unable to precompile insert stmt: %s\n", sqlite3_errmsg(db));
        
    // Save a copy of the filename for future error messages
    char* tmp = (char*)malloc(strlen(filename)+1);
    if (tmp == 0)
        return Error("Out of memory opening Sqlite file %s\n", tmp);
    strcpy(tmp, filename);
    filename = tmp;
}



bool SqliteLogger::Cleanup()
{
    if (insert != NULL) sqlite3_finalize(insert);
    insert = NULL;
    if (db != NULL) sqlite3_close(db);
    db = NULL;
    if (filename != NULL) free((void*)filename);
    filename = NULL;

    return OK;
}

