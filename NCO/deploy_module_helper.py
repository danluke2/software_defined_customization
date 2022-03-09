
import argparse

from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(description='Module deploy program')
parser.add_argument('--module', type=str, required=True, help="Module you want to deploy")
parser.add_argument('--host', type=int, required=True, help="Host to deploy module to")


args = parser.parse_args()


db_connection = db_connect(cfg.git_dir + 'cib.db')
insert_req_build_module(db_connection, args.host, args.module)
