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

    init_retired_table(con)

    init_available_modules_table(con)

    init_require_build_module_table(con)

    init_built_modules_table(con)

    # init_command_table(con)
    #
    # init_completed_cmd_table(con)

    # init_install_table(con)

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
            con.execute("INSERT INTO hosts VALUES (?, ?, ?, ?, ?, ?, ?, ?)", (mac,
                        host_id, host_ip, host_port, symvers_ts, config_ts, version, interval))
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
            con.execute("UPDATE hosts SET {} = :col_value WHERE mac = :mac;".format(
                col_name), {"col_value": col_value, "mac": mac})
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
        print(f"Error selecting all hosts")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** ACTIVE TABLE ***********************

def init_active_table(con):
    # each id and module pair must be unique
    con.execute('''CREATE TABLE active
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   sock_count integer, registered_ts integer,
                   host_error_ts integer,
                   PRIMARY KEY (host_id, module_id))''')


def insert_active(con, host_id, module_id, count, registered_ts, host_error_ts):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO active VALUES (?, ?, ?, ?, ?)",
                        (host_id, module_id, count, registered_ts, host_error_ts))
    except sl.Error as er:
        print(
            f"Error inserting module into active, module_id = {module_id}, host_id = {host_id}")
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
        print(
            f"Error updating active row, module_id = {module_id}, host_id = {host_id}")
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
        print(
            f"Error updating active error, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_active(con, host_id, module_id):
    result = 1
    try:
        with con:
            con.execute("DELETE FROM active WHERE host_id = :host AND module_id =:module;", {
                        "host": host_id, "module": module_id})
    except sl.Error as er:
        print(
            f"Error deleting active row, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_active_modules(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            result = [mod_id[0] for mod_id in cur.execute(
                "SELECT module_id FROM active WHERE host_id =:id;", {"id": host_id})]
    except sl.Error as er:
        print(f"Error selecting active modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** RETIRED TABLE ***********************

def init_retired_table(con):
    # retired table can have multiple rows with same id or module
    con.execute('''CREATE TABLE retired
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   retired_ts integer NOT NULL)''')


def insert_retired(con, host_id, module_id, ts):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO retired VALUES (?, ?, ?)",
                        (host_id, module_id, ts))
    except sl.Error as er:
        print(
            f"Error inserting module into retired, module_id = {module_id}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_retired(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM retired WHERE host_id = :id AND module_id =:module;", {
                        "id": host_id, "module": module_id})
    except sl.Error as er:
        print(
            f"Error deleting retired row, module={module_id}, host={host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** AVAIL_MOD TABLE ***********************

def init_available_modules_table(con):
    # these are core modules available for build and install, thus module name is unique
    con.execute('''CREATE TABLE available_modules
                   (module text NOT NULL PRIMARY KEY, application text,
                   src_ip text, src_port integer,
                   dest_ip text, dest_port integer,
                   L4_proto integer, description text)''')


def insert_available_module(con, module, app, src_ip, src_port, dest_ip, dest_port, l4, desc):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO available_modules VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                        (module, app, src_ip, src_port, dest_ip, dest_port, l4, desc))
    except sl.Error as er:
        print(f"Error inserting available_modules module = {module}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_available_module(con, module):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM available_modules WHERE module =:module;", {
                        "module": module})
    except sl.Error as er:
        print(f"Error deleting module, module = {module}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** REQUIRE_BUILD_MOD TABLE ***********************

def init_require_build_module_table(con):
    con.execute('''CREATE TABLE req_build_modules
                   (host_id integer NOT NULL, module text NOT NULL,
                   PRIMARY KEY (host_id, module))''')


def insert_req_build_module(con, host_id, module):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO req_build_modules VALUES (?, ?)", (host_id, module))
    except sl.Error as er:
        print(
            f"Error inserting req_build_modules module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_build_module(con, module, host_id):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM req_build_modules WHERE host_id = :id AND module =:module;", {
                        "module": module, "id": host_id})
    except sl.Error as er:
        print(
            f"Error deleting required module, module = {module}, host_id = {host_id}")
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
            con.execute("INSERT INTO built_modules VALUES (?, ?, ?, ?, ?, ?, ?)",
                        (host_id, module, module_id, make_ts, key, req_install, ts))
    except sl.Error as er:
        print(
            f"Error inserting built_modules module = {module}, host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM built_modules WHERE host_id = :id AND module =:module;", {
                        "module": module, "id": host_id})
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
                        {"mod_id": module_id, "host": host_id, "req": req_install, "ts_value": ts})
    except sl.Error as er:
        print(
            f"Error updating built_modules row, host = {host_id}, module_id={module_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM built_modules WHERE host_id =:id AND module = :mod;",
                        {"id": host_id, "mod": module})
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
            result = cur.execute(
                "SELECT * FROM built_modules WHERE host_id =:id AND req_install = :req;", {"id": host_id, "req": req_install})
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
            cur.execute("SELECT module_key FROM built_modules WHERE host_id =:id AND module_id = :mod;", {
                        "id": host_id, "mod": module_id})
            result = cur.fetchone()[0]
    except sl.Error as er:
        print(f"Error checking built modules for host_id = {host_id}")
        print(f"Error = {er}")
        result = DB_ERROR
    return result


# Not implemented yet
# ***************** CMD TABLE ***********************
#
# def init_command_table(con):
#     # table to hold commnads to send to host and timestamps for tracking status
#     con.execute('''CREATE TABLE cmd_queue
#                    (host_id integer NOT NULL, cmd text NOT NULL,
#                    deliver_ts integer NOT NULL,
#                    PRIMARY KEY (host_id, cmd))''')
#
#
# def insert_command(con, host_id, cmd, deliver_ts):
#     result = 0
#     try:
#         with con:
#             con.execute("INSERT INTO cmd_queue VALUES (?, ?, ?)", (host_id, cmd, deliver_ts))
#     except sl.Error as er:
#         print(f"Error inserting cmd = {cmd}, host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# def delete_command(con, host_id, cmd):
#     result = 0
#     try:
#         with con:
#             con.execute("DELETE FROM cmd_queue WHERE host_id =:id AND cmd = :cmd;", {"id": host_id, "cmd": cmd})
#     except sl.Error as er:
#         print(f"Error deleting cmd, cmd = {cmd}, host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# def select_commands(con, host_id, deliver_ts):
#     result = 0
#     try:
#         with con:
#             cur = con.cursor()
#             result = cur.execute("SELECT * FROM cmd_queue WHERE host_id =:id AND deliver_ts < :ts;", {"id": host_id, "ts": deliver_ts})
#             result = cur.fetchall()
#     except sl.Error as er:
#         print(f"Error selecting commands for host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result


#  Not implemented yet
# ***************** COMPLETED_CMD TABLE ***********************
#
# def init_completed_cmd_table(con):
#     # table to hold commnads to send to host and timestamps for tracking status
#     con.execute('''CREATE TABLE cmd_completed
#                    (host_id integer NOT NULL, cmd text NOT NULL,
#                    delivered_ts integer,
#                    PRIMARY KEY (host_id, cmd))''')
#
#
# def insert_completed_command(con, host_id, cmd, delivered_ts):
#     result = 0
#     try:
#         with con:
#             con.execute("INSERT INTO cmd_completed VALUES (?, ?, ?)", (host_id, cmd, delivered_ts))
#     except sl.Error as er:
#         print(f"Error inserting cmd = {cmd}, host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# def delete_completed_command(con, host_id, cmd):
#     result = 0
#     try:
#         with con:
#             con.execute("DELETE FROM cmd_completed WHERE host_id =:id AND cmd = :cmd;", {"id": host_id, "cmd": cmd})
#     except sl.Error as er:
#         print(f"Error deleting cmd, cmd = {cmd}, host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result


# Install table integrated into built module table for ease of management
# ***************** INSTALL TABLE ***********************
#
# def init_install_table(con):
#     # install table can have multiple rows with same id or module
#     con.execute('''CREATE TABLE install
#                    (host_id integer NOT NULL, module text NOT NULL,
#                    module_id integer, ts_to_install integer)''')
#
#
# def insert_install(con, host_id, module, module_id, ts_to_install):
#     result = 0
#     try:
#         con.execute("INSERT INTO install VALUES (?, ?, ?, ?)", (host_id, module, module_id, ts_to_install))
#     except sl.Error as er:
#         print(f"Error inserting module into install, module = {module}, host = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# #hosts only report module ID, so we need a column to match against
# def update_install_module_id(con, host_id, module, module_id):
#     result = 0
#     try:
#         with con:
#             con.execute('''UPDATE install
#             SET module_id = :id
#             WHERE host_id = :host AND module =:module;''',
#             {"id": module_id, "host": host_id, "module": module})
#     except sl.Error as er:
#         print(f"Error updating install row, host = {host_id}, module={module}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result

#
# def delete_install_by_module(con, host_id, module):
#     result = 0
#     try:
#         con.execute("DELETE FROM install WHERE host_id = :host AND module =:module;", {"host": host_id, "module": module})
#     except sl.Error as er:
#         print(f"Error deleting retired row, module={module}, host={host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
# def delete_install_by_module_id(con, host_id, module_id):
#     result = 0
#     try:
#         con.execute("DELETE FROM install WHERE host_id = :host AND module_id =:id;", {"host": host_id, "id": module_id})
#     except sl.Error as er:
#         print(f"Error deleting retired row, module_id={module_id}, host={host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
#
#
#
# def select_install_modules(con, host_id):
#     result = 0
#     try:
#         with con:
#             cur = con.cursor()
#             result = cur.execute("SELECT * FROM install WHERE host_id =:id;", {"id": host_id})
#             result = cur.fetchall()
#     except sl.Error as er:
#         print(f"Error selecting install modules for host_id = {host_id}")
#         print(f"Error = {er}")
#         result = DB_ERROR
#     return result
