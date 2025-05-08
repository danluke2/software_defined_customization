#!/usr/bin/env python3

import sqlite3 as sl
from os.path import exists
import logging

DB_ERROR = -1

logger = logging.getLogger(__name__)  # use module name

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

    init_deployed_table(con)

    init_revoked_table(con)

    init_require_build_module_table(con)

    init_require_active_toggle_table(con)

    init_require_set_priority_table(con)

    init_require_deprecate_table(con)

    init_require_revocation_table(con)

    init_built_modules_table(con)

    init_build_error_table(con)

    init_middlebox_table(con)

    init_inverse_module_table(con)

    init_deploy_inverse_table(con)

    init_alert_table(con)

    # init_available_modules_table(con)

    return


def drop_table(con, table):
    command = "DROP TABLE " + table + ";"
    con.execute(command)


# ***************** ALERT TABLE ***********************
def init_alert_table(con):
    # alert table can have multiple rows with same id or module
    con.execute(
        """
                CREATE TABLE alerts (
                    host_id INTEGER NOT NULL,
                    alert_data TEXT NOT NULL,
                    UNIQUE(host_id, alert_data)
                );
            """
    )


def insert_alert(con, host_id, alert_data):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT OR IGNORE INTO alerts (host_id, alert_data) VALUES (?, ?)",
                (host_id, alert_data),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting alert for host_id = {host_id}, alert_data = {alert_data}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_alert(con, host_id, alert_data):
    # host_id is an integer, alert_data is a string
    result = 0
    try:
        with con:
            con.execute(
                "UPDATE alerts SET alert_data = ? WHERE host_id = ?",
                (alert_data, host_id),
            )
    except sl.Error as er:
        logger.info(
            f"Error updating alert for host_id = {host_id}, alert_data = {alert_data}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_alerts(con):
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT host_id, alert_data FROM alerts;")
            rows = cur.fetchall()

            host_alerts = {}
            for host_id, alert_data in rows:
                host_alerts.setdefault(host_id, []).append(alert_data)

            return host_alerts
    except sl.Error as er:
        logger.info("Error selecting all alerts")
        logger.info(f"Error = {er}")
        return {}


# ***************** POLICY TABLE *********************


def init_policy_table(con):
    # policy table records policies deployed
    con.execute(
        """CREATE TABLE policies
                   (cpcon_level text NOT NULL,
                     threat text, action text, host_id integer)"""
    )


def insert_policy(con, cpcon_level, threat, action, host_id):
    # cpcon_level is a string, threat is a string, action is a string, host_id is an integer
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO policies (cpcon_level, threat, action, host_id) VALUES (?, ?, ?, ?)",
                (cpcon_level, threat, action, host_id),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting policy for host_id = {host_id}, cpcon_level = {cpcon_level}, threat = {threat}, action = {action}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_policy(con, cpcon_level, threat, action, host_id):
    # cpcon_level is a string, threat is a string, action is a string, host_id is an integer
    result = 0
    try:
        with con:
            con.execute(
                "UPDATE policies SET cpcon_level = ?, threat = ?, action = ? WHERE host_id = ?",
                (cpcon_level, threat, action, host_id),
            )
    except sl.Error as er:
        logger.info(
            f"Error updating policy for host_id = {host_id}, cpcon_level = {cpcon_level}, threat = {threat}, action = {action}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def view_all_policies(con):
    # cpcon_level is a string, threat is a string, action is a string, host_id is an integer
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM policies;")
            result = cur.fetchall()
            if not result:  # Check if the result is empty
                logger.info("No policies found in the database.")
                result = None
    except sl.Error as er:
        logger.info(f"Error selecting all policies")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** HOST TABLE ***********************


def init_host_table(con):
    # host table uses primary mac to associate to host and store host info
    con.execute(
        """CREATE TABLE hosts
                   (mac text NOT NULL PRIMARY KEY, host_id integer,
                   host_ip text, host_port text,
                   symvers_ts integer, config_ts integer,
                   linux_version text, interval integer)"""
    )


def insert_host(
    con, mac, host_id, host_ip, host_port, symvers_ts, config_ts, version, interval
):
    result = 0
    # Successful, con.commit() is called automatically afterwards
    # con.rollback() is called after the with block finishes with an exception, the
    # exception is still raised and must be caught
    try:
        with con:
            con.execute(
                "INSERT INTO hosts VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (
                    mac,
                    host_id,
                    host_ip,
                    host_port,
                    symvers_ts,
                    config_ts,
                    version,
                    interval,
                ),
            )
    except sl.Error as er:
        logger.info(f"Error inserting host mac = {mac}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# should only update ts  and interval values, but could be other updates occasionally
def update_host(con, mac, col_name, col_value):
    result = 0
    try:
        with con:
            con.execute(
                "UPDATE hosts SET {} = :col_value WHERE mac = :mac;".format(col_name),
                {"col_value": col_value, "mac": mac},
            )
    except sl.Error as er:
        logger.info(f"Error updating host column = {col_name}, value = {col_value}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_host(con, mac):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM hosts WHERE mac =:mac;", {"mac": mac})
    except sl.Error as er:
        logger.info(f"Error deleting host, mac = {mac}")
        logger.info(f"Error = {er}")
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
        logger.info(f"Error selecting host, mac = {mac}")
        logger.info(f"Error = {er}")
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
        logger.info(f"Error selecting all hosts")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** DEPLOYED TABLE ***********************


def init_deployed_table(con):
    # each id and module pair must be unique
    con.execute(
        """CREATE TABLE deployed
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   sock_count integer, active_mode integer,
                   security_window integer, last_security_ts integer,
                   registered_ts integer, deprecated_ts integer, host_error_ts integer,
                   PRIMARY KEY (host_id, module_id))"""
    )


def insert_deployed(
    con,
    host_id,
    module_id,
    count,
    active_mode,
    sec_window,
    sec_ts,
    registered_ts,
    deprecated_ts,
    host_error_ts,
):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO deployed VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (
                    host_id,
                    module_id,
                    count,
                    active_mode,
                    sec_window,
                    sec_ts,
                    registered_ts,
                    deprecated_ts,
                    host_error_ts,
                ),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting module into deployed, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_deployed(con, host_id, module_id, count, active_mode, registered_ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deployed
            SET sock_count = :count, active_mode = :active_mode, registered_ts = :reg
            WHERE host_id = :host AND module_id =:module;""",
                {
                    "count": count,
                    "active_mode": active_mode,
                    "reg": registered_ts,
                    "host": host_id,
                    "module": module_id,
                },
            )
    except sl.Error as er:
        logger.info(
            f"Error updating deployed row, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_deployed_sec_window(con, host_id, module_id, sec_window):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deployed
            SET security_window = :window
            WHERE host_id = :host AND module_id =:module;""",
                {"window": sec_window, "host": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating deployed sec window, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_deployed_sec_ts(con, host_id, module_id, sec_ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deployed
            SET last_security_ts = :ts
            WHERE host_id = :host AND module_id =:module;""",
                {"ts": sec_ts, "host": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating deployed sec window, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_deployed_deprecated_ts(con, host_id, module_id, deprecated_ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deployed
            SET deprecated_ts = :ts
            WHERE host_id = :host AND module_id =:module;""",
                {"ts": deprecated_ts, "host": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating deployed deprecated_ts, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_deployed_host_error(con, host_id, module_id, host_error_ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deployed
            SET host_error_ts = :ts
            WHERE host_id = :host AND module_id =:module;""",
                {"ts": host_error_ts, "host": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating deployed error, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_deployed(con, host_id, module_id):
    result = 1
    try:
        with con:
            con.execute(
                "DELETE FROM deployed WHERE host_id = :host AND module_id =:module;",
                {"host": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting deployed row, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_deployed_modules(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            result = [
                mod_id[0]
                for mod_id in cur.execute(
                    "SELECT module_id FROM deployed WHERE host_id =:id;",
                    {"id": host_id},
                )
            ]
    except sl.Error as er:
        logger.info(f"Error selecting deployed modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_deployed_rows(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM deployed WHERE host_id =:id;", {"id": host_id})
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error selecting deployed modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** ACTIVE TOGGLE TABLE ***********************


def init_require_active_toggle_table(con):
    # each id and module pair must be unique
    con.execute(
        """CREATE TABLE req_active_toggle
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                    mode integer NOT NULL,
                    PRIMARY KEY (host_id, module_id))"""
    )


def insert_req_active_toggle(con, host_id, module_id, mode):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO req_active_toggle VALUES (?, ?, ?)",
                (host_id, module_id, mode),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting module into req_active_toggle, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_active_toggle_by_id(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_active_toggle WHERE host_id = :id AND module_id =:module;",
                {"id": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting req_active_toggle row, module={module_id}, host={host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_active_toggle(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM req_active_toggle WHERE host_id = :id;", {"id": host_id}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error slecting all req_active_toggle for host {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** SET PRIORITY TABLE ***********************


def init_require_set_priority_table(con):
    # each id and module pair must be unique
    con.execute(
        """CREATE TABLE req_set_priority
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                    priority integer NOT NULL,
                    PRIMARY KEY (host_id, module_id))"""
    )


def insert_req_set_priority(con, host_id, module_id, priority):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO req_set_priority VALUES (?, ?, ?)",
                (host_id, module_id, priority),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting module into req_set_priority, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_set_priority_by_id(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_set_priority WHERE host_id = :id AND module_id =:module;",
                {"id": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting req_set_priority row, module={module_id}, host={host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_set_priority(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM req_set_priority WHERE host_id = :id;", {"id": host_id}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error slecting all req_set_priority for host {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** DEPRECATE TABLE ***********************


def init_require_deprecate_table(con):
    # each id and module pair must be unique
    con.execute(
        """CREATE TABLE req_deprecate
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   PRIMARY KEY (host_id, module_id))"""
    )


def insert_req_deprecate(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute("INSERT INTO req_deprecate VALUES (?, ?)", (host_id, module_id))
    except sl.Error as er:
        logger.info(
            f"Error inserting module into req_deprecate, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_deprecate_by_id(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_deprecate WHERE host_id = :id AND module_id =:module;",
                {"id": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting req_deprecate row, module={module_id}, host={host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_deprecate(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM req_deprecate WHERE host_id = :id;", {"id": host_id}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error slecting all req_deprecate for host {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** REQ REVOCATION TABLE ***********************


def init_require_revocation_table(con):
    # revoked table can have multiple rows with same id or module
    con.execute(
        """CREATE TABLE req_revocation
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   module text NOT NULL)"""
    )


def insert_req_revocation(con, host_id, module_id, mod_name):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO req_revocation VALUES (?, ?, ?)",
                (host_id, module_id, mod_name),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting module into req_revocation, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_revocation_by_id(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_revocation WHERE host_id = :id AND module_id =:module;",
                {"id": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting revocation row, module={module_id}, host={host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_revocation_by_name(con, host_id, module_name):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_revocation WHERE host_id = :id AND module =:module;",
                {"id": host_id, "module": module_name},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting revocation row, module={module_name}, host={host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_req_revocation(con, host_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM req_revocation WHERE host_id = :id;", {"id": host_id}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error slecting all required revocations for host {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** REVOKED TABLE ***********************


def init_revoked_table(con):
    # revoked table can have multiple rows with same id or module
    con.execute(
        """CREATE TABLE revoked
                   (host_id integer NOT NULL, module_id integer NOT NULL,
                   revoked_ts integer NOT NULL)"""
    )


def insert_revoked(con, host_id, module_id, ts):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO revoked VALUES (?, ?, ?)", (host_id, module_id, ts)
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting module into revoked, module_id = {module_id}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_revoked(con, host_id, module_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM revoked WHERE host_id = :id AND module_id =:module;",
                {"id": host_id, "module": module_id},
            )
    except sl.Error as er:
        logger.info(f"Error deleting revoked row, module={module_id}, host={host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** REQUIRE_BUILD_MOD TABLE ***********************
# TODO: add an error column to prevent building over and over if error exists
# IDS: added alert column
def init_require_build_module_table(con):
    con.execute(
        """CREATE TABLE req_build_modules
                   (host_id integer NOT NULL, module text NOT NULL,
                    active_mode integer NOT NULL, priority integer NOT NULL,
                    applyNow integer NOT NULL, alert integer NOT NULL,
                   PRIMARY KEY (host_id, module))"""
    )


def insert_req_build_module(con, host_id, module, active_mode, priority, apply, alert):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO req_build_modules VALUES (?, ?, ?, ?, ?, ?)",
                (host_id, module, active_mode, priority, apply, alert),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting req_build_modules module = {module}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_req_build_module(con, module, host_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM req_build_modules WHERE host_id = :id AND module =:module;",
                {"module": module, "id": host_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting required module, module = {module}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
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
        logger.info(f"Error slecting all required build modules")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** BUILT_MOD TABLE ***********************


def init_built_modules_table(con):
    # module, id pair should be unique since we only keep latest build
    con.execute(
        """CREATE TABLE built_modules
                   (host_id integer NOT NULL, module text NOT NULL,
                   module_id integer NOT NULL, make_ts integer NOT NULL,
                   module_key BLOB NOT NULL,
                   req_install integer NOT NULL, ts_to_install integer,
                   PRIMARY KEY (host_id, module))"""
    )


def insert_built_module(con, host_id, module, module_id, make_ts, key, req_install, ts):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO built_modules VALUES (?, ?, ?, ?, ?, ?, ?)",
                (host_id, module, module_id, make_ts, key, req_install, ts),
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting built_modules module = {module}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM built_modules WHERE host_id = :id AND module =:module;",
                {"module": module, "id": host_id},
            )
    except sl.Error as er:
        logger.info(f"Error deleting module, module = {module}, host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_built_module_install_requirement(con, host_id, module_id, req_install, ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE built_modules
            SET req_install = :req, ts_to_install = :ts_value
            WHERE host_id = :host AND module_id =:mod_id ;""",
                {
                    "mod_id": module_id,
                    "host": host_id,
                    "req": req_install,
                    "ts_value": ts,
                },
            )
    except sl.Error as er:
        logger.info(
            f"Error updating built_modules row, host = {host_id}, module_id={module_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module(con, host_id, module):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM built_modules WHERE host_id =:id AND module = :mod;",
                {"id": host_id, "mod": module},
            )
            result = cur.fetchone()
    except sl.Error as er:
        logger.info(f"Error checking built modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module_by_id(con, host_id, mod_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM built_modules WHERE host_id =:id AND module_id = :mod_id;",
                {"id": host_id, "mod_id": mod_id},
            )
            result = cur.fetchone()
    except sl.Error as er:
        logger.info(f"Error checking built modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_modules_to_install(con, host_id, req_install):
    result = 0
    try:
        with con:
            cur = con.cursor()
            result = cur.execute(
                "SELECT * FROM built_modules WHERE host_id =:id AND req_install = :req;",
                {"id": host_id, "req": req_install},
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error selecting install modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_built_module_key(con, host_id, module_id):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT module_key FROM built_modules WHERE host_id =:id AND module_id = :mod;",
                {"id": host_id, "mod": module_id},
            )
            result = cur.fetchone()[0]
    except sl.Error as er:
        logger.info(f"Error checking built modules for host_id = {host_id}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** BUILD_ERROR TABLE ***********************
# TODO: add an error column to prevent building over and over if error exists


def init_build_error_table(con):
    con.execute(
        """CREATE TABLE build_error
                   (host_id integer NOT NULL, module text NOT NULL,
                   error text NOT NULL,
                   PRIMARY KEY (host_id, module))"""
    )


def insert_build_error(con, host_id, module, error):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO build_error VALUES (?, ?, ?)", (host_id, module, error)
            )
    except sl.Error as er:
        logger.info(
            f"Error inserting build_error module = {module}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_build_error(con, module, host_id):
    result = 0
    try:
        with con:
            con.execute(
                "DELETE FROM build_error WHERE host_id = :id AND module =:module;",
                {"module": module, "id": host_id},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting build_error module, module = {module}, host_id = {host_id}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_build_error_modules(con):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM build_error;")
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error slecting all build error modules")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** MIDDLEBOX (NON-L4.5) TABLE ***********************
# Table to hold middlebox info to allow matching inverse modules for deployment


def init_middlebox_table(con):
    # host table uses primary mac to associate to host and store host info
    con.execute(
        """CREATE TABLE middlebox
                   (mac text NOT NULL, mid_id integer,
                   ip text, port text, type text,
                   linux_version text, interval integer,
                   PRIMARY KEY (mac, mid_id))"""
    )


def insert_middlebox(con, mac, mid_id, ip, port, type, version, interval):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO middlebox VALUES (?, ?, ?, ?, ?, ?, ?)",
                (mac, mid_id, ip, port, type, version, interval),
            )
    except sl.Error as er:
        logger.info(f"Error inserting middlebox mac = {mac}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# should only update interval values, but could be other updates occasionally
def update_middlebox(con, mac, col_name, col_value):
    result = 0
    try:
        with con:
            con.execute(
                "UPDATE middlebox SET {} = :col_value WHERE mac = :mac;".format(
                    col_name
                ),
                {"col_value": col_value, "mac": mac},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating middlebox column = {col_name}, value = {col_value}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_middlebox(con, mac):
    result = 0
    try:
        with con:
            con.execute("DELETE FROM middlebox WHERE mac =:mac;", {"mac": mac})
    except sl.Error as er:
        logger.info(f"Error deleting middlebox, mac = {mac}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_middlebox(con, mac):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM middlebox WHERE mac =:mac;", {"mac": mac})
            result = cur.fetchone()
    except sl.Error as er:
        logger.info(f"Error selecting middlebox, mac = {mac}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_all_middleboxes(con):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM middlebox;")
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error selecting all middleboxes")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_middlebox_by_type(con, type):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM middlebox WHERE type = :type;", {"type": type})
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error selecting middlebox, type = {type}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** MODULE-INVERSE TABLE ***********************
# Table to correlate a Layer 4.5 module with its inverse module and what type
# of middlebox it needs to be deployed on


def init_inverse_module_table(con):
    con.execute(
        """CREATE TABLE inverse_modules
                   (module text NOT NULL, inverse text NOT NULL,
                   type text NOT NULL,
                   PRIMARY KEY (module, inverse))"""
    )


def insert_inverse_module(con, module, inverse, type):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO inverse_modules VALUES (?, ?, ?)", (module, inverse, type)
            )
    except sl.Error as er:
        logger.info(f"Error inserting inverse module = {module}, inverse = {inverse}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_inverse_module(con, module, inverse, type):
    result = 0
    try:
        with con:
            con.execute(
                """DELETE FROM inverse_modules
                        WHERE module =:mod AND inverse =:inv AND type=:type;""",
                {"mod": module, "inv": inverse, "type": type},
            )
    except sl.Error as er:
        logger.info(
            f"Error deleting inverse, module = {module}, inverse = {inverse}, type = {type}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_inverse_by_module(con, module):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM inverse_modules WHERE module = :mod;", {"mod": module}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error checking inverse modules for module = {module}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_inverse_by_inverse(con, inverse):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM inverse_modules WHERE inverse = :inv;", {"inv": inverse}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error checking inverse modules for inverse = {inverse}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_inverse_by_type(con, type):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM inverse_modules WHERE type = :type;", {"type": type}
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error checking inverse modules for type = {type}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


# ***************** DEPLOY INVERSE TABLE ***********************
# Table to hold scheduled inverse modules to deploy to indicated middlebox


def init_deploy_inverse_table(con):
    con.execute(
        """CREATE TABLE deploy_inverse
                   (inverse text NOT NULL, middlebox text NOT NULL,
                   req_install int NOT NULL, installed_ts int,
                   PRIMARY KEY (inverse, middlebox))"""
    )


def insert_deploy_inverse(con, inverse, middlebox, require, ts):
    result = 0
    try:
        with con:
            con.execute(
                "INSERT INTO deploy_inverse VALUES (?, ?, ?, ?)",
                (inverse, middlebox, require, ts),
            )
    except sl.Error as er:
        logger.info(f"Error inserting deploy inverse module = {inverse}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def delete_deploy_inverse(con, inverse, middlebox):
    result = 0
    try:
        with con:
            con.execute(
                """DELETE FROM deploy_inverse
                        WHERE inverse =:inv AND middlebox=:mid;""",
                {"inv": inverse, "mid": middlebox},
            )
    except sl.Error as er:
        logger.info(f"Error deleting inverse = {inverse}, middlebox = {middlebox}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def update_inverse_module_installed_status(con, inverse, middlebox, require, ts):
    result = 0
    try:
        with con:
            con.execute(
                """UPDATE deploy_inverse
                        SET req_install = :req, installed_ts = :ts
                        WHERE inverse =:inv AND middlebox=:mid ;""",
                {"inv": inverse, "mid": middlebox, "req": require, "ts": ts},
            )
    except sl.Error as er:
        logger.info(
            f"Error updating inverse row, inverse = {inverse}, middlebox = {middlebox}"
        )
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_deploy_inverse_by_module(con, inverse, require):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM deploy_inverse WHERE inverse = :mod AND req_install = :req;",
                {"mod": inverse, "req": require},
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error checking inverse modules for module = {inverse}")
        logger.info(f"Error = {er}")
        result = DB_ERROR
    return result


def select_deploy_inverse_by_ip(con, middlebox):
    result = 0
    try:
        with con:
            cur = con.cursor()
            cur.execute(
                "SELECT * FROM deploy_inverse WHERE middlebox = :mid;",
                {"mid": middlebox},
            )
            result = cur.fetchall()
    except sl.Error as er:
        logger.info(f"Error checking inverse modules for middlebox = {middlebox}")
        logger.info(f"Error = {er}")
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
#         logger.info(f"Error inserting available_modules module = {module}")
#         logger.info(f"Error = {er}")
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
#         logger.info(f"Error deleting module, module = {module}")
#         logger.info(f"Error = {er}")
#         result = DB_ERROR
#     return result
