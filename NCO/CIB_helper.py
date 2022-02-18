#!/usr/bin/env python3

import sqlite3 as sl
from os.path import exists


DB_ERROR = -1

# Open table if exists and return cursor, if didn't exist, then create and initialize
def db_connect(db_name):
    file_exists = exists(db_name)
    if file_exists:
        con = sl.connect(db_name)
        con.row_factory = sl.Row
    else:
        con = sl.connect(db_name)
        init_db_tables(con)
        con.row_factory = sl.Row

    return con


# Open table if exists and return cursor, if didn't exist, then create and initialize
def db_close(con):
    con.commit()
    con.close()
    return

#  create all the tables PCC needs
def init_db_tables(con):

    init_host_table(con)

    init_active_table(con)

    init_require_revocation_table(con)

    init_revoked_table(con)

    init_require_build_module_table(con)

    init_built_modules_table(con)

    # init_available_modules_table(con)

    return


def drop_table(con, table):
    command = "DROP TABLE " + table + ";"
    con.execute(command)



# ***************** HOST TABLE ***********************

def init_host_table(con):
    # host table uses primary mac to associate to host and store host info
    con.execute('''CREATE TABLE hosts
                   (mac text NOT NULL PRIMARY KEY, host_id integer,
                   host_ip text, host_port text,
                   symvers_ts integer, config_ts integer,
                   linux_version text, interval integer)''')


def insert_host(con, mac, host_id, host_ip, host_port, symvers_ts, config_ts, version, interval):
    result = 0
    # Successful, con.commit() is called automatically afterwards
    # con.rollback() is called after the with block finishes with an exception, the
    # exception is still raised and must be caught
    try:
        with con:
            con.execute("INSERT INTO hosts VALUES (?, ?, ?, ?, ?, ?, ?, ?)", (mac, host_id, host_ip, host_port, symvers_ts, config_ts, version, interval))
    except sl.Error as er:
        print(f"Error inserting host mac = {mac}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# should only update ts  and interval values, but could be other updates occasionally
def update_host(con, mac, col_name, col_value):
    result = 0
    try:
        with con:
            con.execute("UPDATE hosts SET {} = :col_value WHERE mac = :mac;".format(col_name), {"col_value": col_value, "mac": mac})
    except sl.Error as er:
        print(f"Error updating host column = {col_name}, value = {col_value}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_host(con, mac):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM hosts WHERE mac =:mac;", {"mac": mac})
    except sl.Error as er:
        print(f"Error deleting host, mac = {mac}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_host(con, mac):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM hosts WHERE mac =:mac;", {"mac": mac})
            result = cur.fetchone()
    except sl.Error as er:
        print(f"Error selecting host, mac = {mac}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_hosts(con):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM hosts;")
            result = cur.fetchall()
    except sl.Error as er:
        print(f"Error selecting host, mac = {mac}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result




# TODO: this is really the Deployed module table
# ***************** ACTIVE TABLE ***********************

def init_active_table(con):
    # each id and module pair must be unique
    con.execute('''CREATE TABLE active
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   sock_count integer, registered_ts integer,
                   security_window integer, last_security_ts integer,
                   host_error_ts integer,
                   PRIMARY KEY (host_id, module_id))''')


def insert_active(con, host_id, module_id, count, registered_ts, sec_window, sec_ts, host_error_ts):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO active VALUES (?, ?, ?, ?, ?, ?, ?)", (host_id, module_id, count, registered_ts, sec_window, sec_ts, host_error_ts))
    except sl.Error as er:
        print(f"Error inserting module into active, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def update_active(con, host_id, module_id, count, registered_ts):
    result = 0
    try:
        with con:
            con.execute('''UPDATE active
            SET sock_count = :count, registered_ts = :reg
            WHERE host_id = :host AND module_id =:module;''',
            {"count": count, "reg": registered_ts, "host": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error updating active row, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result

def update_active_sec_window(con, host_id, module_id, sec_window):
    result = 0
    try:
        with con:
            con.execute('''UPDATE active
            SET security_window = :window
            WHERE host_id = :host AND module_id =:module;''',
            {"window": sec_window, "host": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error updating active sec window, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def update_active_sec_ts(con, host_id, module_id, sec_ts):
    result = 0
    try:
        with con:
            con.execute('''UPDATE active
            SET last_security_ts = :ts
            WHERE host_id = :host AND module_id =:module;''',
            {"ts": sec_ts, "host": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error updating active sec window, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def update_active_host_error(con, host_id, module_id, host_error_ts):
    result = 0
    try:
        with con:
            con.execute('''UPDATE active
            SET host_error_ts = :ts
            WHERE host_id = :host AND module_id =:module;''',
            {"ts": host_error_ts, "host": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error updating active error, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_active(con, host_id, module_id):
    result = 1
    try:
        with con:
            con.execute("DELETE FROM active WHERE host_id = :host AND module_id =:module;", {"host": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error deleting active row, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_active_modules(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            result = [mod_id[0] for mod_id in cur.execute("SELECT module_id FROM active WHERE host_id =:id;", {"id": host_id})]
    except sl.Error as er:
        print(f"Error selecting active modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_active_rows(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM active WHERE host_id =:id;", {"id": host_id})
            result = cur.fetchall()
    except sl.Error as er:
        print(f"Error selecting active modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result




# ***************** REQ REVOCATION TABLE ***********************

def init_require_revocation_table(con):
    # revoked table can have multiple rows with same id or module
    con.execute('''CREATE TABLE req_revocation
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   module text NOT NULL)''')


def insert_req_revocation(con, host_id, module_id, mod_name):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO req_revocation VALUES (?, ?, ?)", (host_id, module_id, mod_name))
    except sl.Error as er:
        print(f"Error inserting module into req_revocation, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_revocation_by_id(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM req_revocation WHERE host_id = :id AND module_id =:module;", {"id": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error deleting revocation row, module={module_id}, host={host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_revocation_by_name(con, host_id, module_name):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM req_revocation WHERE host_id = :id AND module =:module;", {"id": host_id, "module": module_name})
    except sl.Error as er:
        print(f"Error deleting revocation row, module={module_name}, host={host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_revocation(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM req_revocation WHERE host_id = :id;", {"id": host_id})
            result = cur.fetchall()
    except sl.Error as er:
        print(f"Error slecting all required revocations for host {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** REVOKED TABLE ***********************

def init_revoked_table(con):
    # revoked table can have multiple rows with same id or module
    con.execute('''CREATE TABLE revoked
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   revoked_ts integer NOT NULL)''')


def insert_revoked(con, host_id, module_id, ts):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO revoked VALUES (?, ?, ?)", (host_id, module_id, ts))
    except sl.Error as er:
        print(f"Error inserting module into revoked, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_revoked(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM revoked WHERE host_id = :id AND module_id =:module;", {"id": host_id, "module": module_id})
    except sl.Error as er:
        print(f"Error deleting revoked row, module={module_id}, host={host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result




# ***************** REQUIRE_BUILD_MOD TABLE ***********************
# TODO: add an error column to prevent building over and over if error exists

def init_require_build_module_table(con):
    con.execute('''CREATE TABLE req_build_modules
                   (host_id integer NOT NULL, module text NOT NULL,
                   PRIMARY KEY (host_id, module))''')

def insert_req_build_module(con, host_id, module):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO req_build_modules VALUES (?, ?)", (host_id, module))
    except sl.Error as er:
        print(f"Error inserting req_build_modules module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_build_module(con, module, host_id):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM req_build_modules WHERE host_id = :id AND module =:module;", {"module": module, "id": host_id})
    except sl.Error as er:
        print(f"Error deleting required module, module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_build_modules(con):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM req_build_modules;")
            result = cur.fetchall()
    except sl.Error as er:
        print(f"Error slecting all required build modules")
        print(f"Error = {er}")
        result = DB_ERROR
    return result



# ***************** BUILT_MOD TABLE ***********************

def init_built_modules_table(con):
    # module, id pair should be unique since we only keep latest build
    con.execute('''CREATE TABLE built_modules
                   (host_id integer NOT NULL, module text NOT NULL,
                   module_id integer NOT NULL, make_ts integer NOT NULL,
                   module_key BLOB NOT NULL,
                   req_install integer NOT NULL, ts_to_install integer,
                   PRIMARY KEY (host_id, module))''')


def insert_built_module(con, host_id, module, module_id, make_ts, key, req_install, ts):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO built_modules VALUES (?, ?, ?, ?, ?, ?, ?)", (host_id, module, module_id, make_ts, key, req_install, ts))
    except sl.Error as er:
        print(f"Error inserting built_modules module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM built_modules WHERE host_id = :id AND module =:module;", {"module": module, "id": host_id})
    except sl.Error as er:
        print(f"Error deleting module, module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def update_built_module_install_requirement(con, host_id, module_id, req_install, ts):
    result = 0
    try:
        with con:
            con.execute('''UPDATE built_modules
            SET req_install = :req, ts_to_install = :ts_value
            WHERE host_id = :host AND module_id =:mod_id ;''',
            {"mod_id": module_id, "host": host_id, "req":req_install, "ts_value":ts})
    except sl.Error as er:
        print(f"Error updating built_modules row, host = {host_id}, module_id={module_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM built_modules WHERE host_id =:id AND module = :mod;", {"id": host_id, "mod": module})
            result = cur.fetchone()
    except sl.Error as er:
        print(f"Error checking built modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module_by_id(con, host_id, mod_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM built_modules WHERE host_id =:id AND module_id = :mod_id;", {"id": host_id, "mod_id": mod_id})
            result = cur.fetchone()
    except sl.Error as er:
        print(f"Error checking built modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_modules_to_install(con, host_id, req_install):
    result = 0
    try:
        with con:
            cur = con.cursor()
            result = cur.execute("SELECT * FROM built_modules WHERE host_id =:id AND req_install = :req;", {"id": host_id, "req": req_install})
            result = cur.fetchall()
    except sl.Error as er:
        print(f"Error selecting install modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module_key(con, host_id, module_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT module_key FROM built_modules WHERE host_id =:id AND module_id = :mod;", {"id": host_id, "mod": module_id})
            result = cur.fetchone()[0]
    except sl.Error as er:
        print(f"Error checking built modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result





# # ***************** AVAIL_MOD TABLE ***********************
#
# def init_available_modules_table(con):
#     # these are core modules available for build and install, thus module name is unique
#     con.execute('''CREATE TABLE available_modules
#                    (module text NOT NULL PRIMARY KEY, application text,
#                    src_ip text, src_port integer,
#                    dest_ip text, dest_port integer,
#                    L4_proto integer, description text)''')
#
#
# def insert_available_module(con, module, app, src_ip, src_port, dest_ip, dest_port, l4, desc):
#     result = 0
#     try:
#         with con:
#             con.execute("INSERT INTO available_modules VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
#             (module, app, src_ip, src_port, dest_ip, dest_port, l4, desc))
#     except sl.Error as er:
#         print(f"Error inserting available_modules module = {module}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# def delete_available_module(con, module):
#     result = 0
#     try:
#         with con:
#             con.execute("DELETE FROM available_modules WHERE module =:module;", {"module": module})
#     except sl.Error as er:
#         print(f"Error deleting module, module = {module}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
