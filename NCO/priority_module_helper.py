import argparse
import time


from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(
    description='Module set priority program')
parser.add_argument('--module', type=str, required=True,
                    help="Module you want to set priority for")
parser.add_argument('--host', type=int, required=True,
                    help="Host to deploy module to")
parser.add_argument('--priority', type=int, required=True,
                    help="New priority level")


args = parser.parse_args()

db_connection = db_connect(cfg.nco_dir + 'cib.db')

module = select_built_module(db_connection, args.host, args.module)
id = module[2]


insert_req_set_priority(db_connection, args.host, id, args.priority)
