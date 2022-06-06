import argparse
import time


from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(
    description='Module activate program')
parser.add_argument('--module', type=str, required=True,
                    help="Module you want to toggle active status")
parser.add_argument('--host', type=int, required=True,
                    help="Host to deploy module to")
parser.add_argument(
    '--activate', help="Toggle module to enable customization", action="store_true")


args = parser.parse_args()

active_mode = 0  # default active_mode off (i.e., no customization applied)

db_connection = db_connect(cfg.nco_dir + 'cib.db')

module = select_built_module(db_connection, args.host, args.module)
id = module[2]


if args.activate:
    active_mode = 1


insert_req_active_toggle(db_connection, args.host, id, active_mode)
