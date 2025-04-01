# ************** STANDARD PARAMS MUST GO HERE ****************
HOST = "10.0.0.20"
nco_dir = "/home/vagrant/software_defined_customization/NCO/"
nco_mod_dir = (
    "/home/vagrant/software_defined_customization/layer4_5_modules/nco_modules/"
)
common_struct_dir = "/home/vagrant/software_defined_customization/DCA_kernel/"
# ************** END STANDARD PARAMS ****************


PORT = 65432  # Port to listen on (non-privileged ports are > 1023)
MIDDLE_PORT = 65433
QUERY_INTERVAL = 30
BUILD_INTERVAL = 5  # run construct loop every X seconds
MAX_BUFFER_SIZE = 4096
SEC_WINDOW = 0


KEY_SIZE = 32
IV_SIZE = 16

random_hosts = True
ip_hosts = False
next_module_id = 1

# default values that builder can overwrite
active_mode = 0  # i.e., no customization applied
applyNow = 0

symvers_dir = "device_modules/host_"  # + host_id
inverse_dir = "inverse_modules/"

log_console = False
log_file = nco_dir + "nco_messages.log"
log_size = 1024 * 8 * 32  # 32KB
log_max = 5  # keep max of 5 logs

# signal codes
REFRESH_INSTALL_LIST = 1
DB_ERROR = -1
CLOSE_SOCK = -2
REVOKE_MOD = -3
REVOKE_ERROR = -4
MIDDLEBOX_ERROR = -5
