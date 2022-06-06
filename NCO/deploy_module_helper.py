
import argparse
import time


from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(description='Module deploy program')
parser.add_argument('--module', type=str, required=True,
                    help="Module you want to deploy")
parser.add_argument('--host', type=int, required=True,
                    help="Host to deploy module to")
parser.add_argument('--inverse', type=str, required=False,
                    help="Module middlebox dependency")
parser.add_argument('--type', type=str, required=False,
                    help="type of middlebox for dependency")
parser.add_argument(
    '--activate', help="Activates module", action="store_true")
parser.add_argument(
    '--applyNow', help="Apply module check to all sockets, including previously checked", action="store_true")

args = parser.parse_args()

active_mode = cfg.active_mode  # default value
applyNow = cfg.applyNow  # default value


db_connection = db_connect(cfg.nco_dir + 'cib.db')

if args.inverse:
    if args.type:
        insert_inverse_module(db_connection, args.module,
                              args.inverse, args.type)

if args.activate:
    active_mode = 1  # overwrite default

if args.applyNow:
    applyNow = 1  # overwrite default

time.sleep(5)

insert_req_build_module(db_connection, args.host,
                        args.module, active_mode, applyNow)
