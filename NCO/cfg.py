HOST = '10.0.0.20'
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)
MIDDLE_PORT = 65433
QUERY_INTERVAL = 30
INSERT_LINE = 42
BUILD_INTERVAL = 10 #run construct loop every 10 seconds
MAX_BUFFER_SIZE = 4096
SEC_WINDOW = 0


KEY_SIZE = 32
IV_SIZE = 16

random_hosts = True
next_module_id = 1
git_dir = "/home/dan/software_defined_customization/NCO/"
core_mod_dir = "core_modules/"
symvers_dir = "device_modules/host_" # + host_id

inverse_dir = "inverse_modules/"

#signal codes
REFRESH_INSTALL_LIST = 1
DB_ERROR = -1
CLOSE_SOCK = -2
REVOKE_MOD = -3
REVOKE_ERROR = -4
MIDDLEBOX_ERROR = -5
