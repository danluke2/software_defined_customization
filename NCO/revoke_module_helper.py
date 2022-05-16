import argparse
import time


from CIB_helper import *
import cfg


parser = argparse.ArgumentParser(description='Module revoke program')
parser.add_argument('--module', type=str, required=True,
                    help="Module you want to revoke")
parser.add_argument('--host', type=int, required=True,
                    help="Host to deploy module to")
parser.add_argument(
    '--deprecate', help="Deprecates module instead of full revoke", action="store_true")


args = parser.parse_args()


db_connection = db_connect(cfg.nco_dir + 'cib.db')


module = select_built_module(db_connection, args.host, args.module)
id = module[2]

if args.deprecate:
    insert_req_deprecate(db_connection, args.host, id)

else:
    insert_req_revocation(db_connection, args.host, id, args.module)
