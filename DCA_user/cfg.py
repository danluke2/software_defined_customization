# ************** STANDARD PARAMS MUST GO HERE ****************
HOST = '10.0.0.20'
dca_dir = '/home/vagrant/software_defined_customization/DCA_user/'
symver_location = '/usr/lib/modules/5.13.0-35-generic/layer4_5/'
system_release = '5.13.0-35-generic'


# ************** STANDARD PARAMS MUST GO HERE ****************


# HOST = '10.10.0.5' GENI

PORT = 65432        # The port used by the NCO
MIDDLE_PORT = 65433        # The middlebox port used by the NCO
INTERFACE = "enp0s8"
# system_name = platform.system()
# system_release = platform.release()

download_dir = symver_location + "customizations"


max_errors = 5
nco_connect_sleep_time = 10


MAX_BUFFER_SIZE = 1024


middle_type = "Wireshark"
middle_dir = "/usr/lib/x86_64-linux-gnu/wireshark/plugins"

KEY_SIZE = 32
IV_SIZE = 16


log_console = False
log_size = 1024 * 8 * 32  # 32KB
log_max = 5  # keep max of 5 logs


log_file = dca_dir + "dca_messages.log"
middle_log_file = dca_dir + "middle_dca_messages.log"

# signal codes
