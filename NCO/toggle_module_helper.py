import argparse
import time


from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(description='Module toggle program')
parser.add_argument('--module', type=str, required=True,
                    help="Module you want to revoke")
parser.add_argument('--host', type=int, required=True,
                    help="Host to deploy module to")
parser.add_argument(
    '--standby', help="Toggle module to standby mode", action="store_true")


args = parser.parse_args()

toggle = 0  # default toggle standby off

db_connection = db_connect(cfg.nco_dir + 'cib.db')


module = select_built_module(db_connection, args.host, args.module)
id = module[2]


if args.standby:
    toggle = 1


insert_req_toggle(db_connection, args.host, id, toggle)
