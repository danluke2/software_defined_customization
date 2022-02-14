HOST = '10.0.0.40'
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)
QUERY_INTERVAL = 30
INSERT_LINE = 42
BUILD_INTERVAL = 10 #run construct loop every 10 seconds
MAX_BUFFER_SIZE = 4096
SEC_WINDOW = 0


KEY_SIZE = 32
IV_SIZE = 16

next_module_id = 1
core_mod_dir = "core_modules/"
symvers_dir = "device_modules/host_" # + host_id


#signal codes
REFRESH_INSTALL_LIST = 1
DB_ERROR = -1
CLOSE_SOCK = -2
