import subprocess
import sys


def valid_int(val):
    """Verify input is an integer type."""

    if val.isnumeric():
        return val

    else:
        valid_int(input("Error: Please provide a valid integer: "))


def machine_input(
    name, box_version, ip_addr, vbox_spec, provider, vmware_spec, memory, cpu
):
    """Populate and return the vagrantfile cofiguration variables for new machine."""

    machines = f"""
  # Multiple machines for running experiment modules
  config.vm.define "{name}" do |{name}|
    {name}.vm.box = "squanchy/layer4.5"
    {name}.vm.box_version = "{box_version}"
    {name}.vm.network "private_network", ip: "{ip_addr}" {vbox_spec}
    {name}.vm.provider "{provider}" do |vb|
      {vmware_spec}
      # Display the GUI when booting the machine
      vb.gui = true
      vb.memory = {memory}
      vb.cpus = {cpu}"""

    return machines


def setup_file(setup_path):
    """Populate and return the Vagrantfile setup_path for specific VM."""

    setup = f"""\n\tconfig.vm.provision :shell, path: "{setup_path}"
end"""

    return setup


def generate_Vagrantfile():
    """Generate a Vagrantfile."""

    ############# STATIC VAGRANTFILE CONTENT #############
    begin = """# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.


  # Single machine for running sample modules
  # config.vm.box =  "squanchy/layer4.5"

"""

    vbox_custom = """
      vb.customize ['modifyvm', :id, '--clipboard', 'bidirectional']
      # vb.customize ["bandwidthctl", :id, "add", "VagrantLimit", "--type", "network", "--limit", "1000m"]
      # vb.customize ["modifyvm", :id, "--nicbandwidthgroup2", "VagrantLimit"]
      #vram setting helps windows 11 machines without necessary processing power
      vb.customize ["modifyvm", :id, "--vram", "128"]
      vb.customize ["modifyvm", :id, "--accelerate3d", "on"]
  """

    comments = """

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # NOTE: This will enable public access to the opened port
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine and only allow access
  # via 127.0.0.1 to disable public access
  # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  # config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  #   vb.customize ['modifyvm', :id, '--clipboard', 'bidirectional']
  #
  #   # Customize the amount of memory on the VM:
  #   # vb.memory = "4096"
  # end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Enable provisioning with a shell script. Additional provisioners such as
  # Ansible, Chef, Docker, Puppet and Salt are also available. Please see the
  # documentation for more information about their specific syntax and use.
  # config.vm.provision "shell", inline: <<-SHELL
  #   apt-get update
  #   apt-get install -y apache2
  # SHELL
  """

    ############# INQUIRY VARIABLES #############
    vm_type = int(
        valid_int(
            input(
                "1) Virtualbox (Intel) \n2) VmWare (ARM, M-Series Mac) \n3) Other (e.g., UTM) \nSelect your virtual machine option above (e.g., 1): "
            )
        )
    )
    # Special case for now since only Virtualbox for Intel and VMWare for ARM are supported
    if vm_type == 3:
        print(
            "Other virtual machine options are not supported at this time. Please refer to manual installation instructions."
        )
        sys.exit()

    # Future Feature: Add support for default values
    vm_num = int(valid_int(input("How many machines would you like to activate?: ")))
    memory = valid_int(input("Enter memory value (MB) in integer (Recommend 8192): "))
    cpu = valid_int(input("Enter CPU count in integer (Recommend 2):"))

    ############# VAGRANTFILE GENERATION #############

    # Variables tailored to vm_type
    if vm_type == 1:
        provider = "virtualbox"
        box_version = "1.2"
        vbox_spec = """, virtualbox__intnet: "layer", nic_type: "virtio" """
        vmware_spec = vbox_custom
        setup_path = "setup.sh"

    elif vm_type == 2:
        provider = "vmware_fusion"
        box_version = "1.3"
        vbox_spec = ""  # Not required for vmware
        vmware_spec = "vb.linked_clone = false"
        setup_path = "setup.sh"

    file_path = "Vagrantfile"

    # Vagrantfile beginning
    with open(file_path, "w") as file:
        file.write(begin)

    # Appending number of machines
    for i in range(vm_num):
        # Update IP Address
        ip_addr = "10.0.0." + str(20 + (i * 10))

        # Update name for each machine
        if i == 0:
            name = "server"
        else:
            name = "client" + str(i)

        # Appending machine config
        with open(file_path, "a") as file:
            file.write(
                machine_input(
                    name,
                    box_version,
                    ip_addr,
                    vbox_spec,
                    provider,
                    vmware_spec,
                    memory,
                    cpu,
                )
            )
            file.write("""\n\t\tend\n\tend\n\n""")

        # if vm_type == 1:
        #     with open(file_path, "a") as file:
        #         file.write(vbox_custom)

    # Appending the comments of Vagrantfile
    with open(file_path, "a") as file:
        file.write(comments)

    with open(file_path, "a") as file:
        file.write(
            """\n\tconfig.vm.synced_folder "C:/Users/Brava/OneDrive - Naval Postgraduate School/NPS/Thesis/TRMC/software_defined_customization", "/home/vagrant/software_defined_customization", owner: "vagrant", group: "vagrant", automount: true\n"""
        )

    # Appending setup.sh path
    with open(file_path, "a") as file:
        file.write(setup_file(setup_path))

    return vm_type


def generate_setup_sh(vm_type):

    ntp_setup = """
# make sure apt is up to date before installer runs
apt update

# give system a little time to sync with ntp
# need to grep for inactive here to reset service if necessary
if timedatectl | grep -q "inactive"; then
    systemctl restart systemd-timesyncd.service
fi

echo "******** Sleeping 20 seconds to allow ntp sync **********"
sleep 20

timedatectl"""

    standard_params = """
# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
# ************** END STANDARD PARAMS ****************
"""

    dns = """
#replace dnsmasq config to match experiments
cp $GIT_DIR/vagrant/dnsmasq.conf /etc/dnsmasq.conf
"""

    aliases = """
# Update .bashrc to include some aliases
cat <<EOT >>/home/vagrant/.bashrc
alias edit='sudo gedit ~/.bashrc'
alias src='source ~/.bashrc'
alias desk='cd /home/vagrant/Desktop'
alias modules='cd /usr/lib/modules/$(uname -r)/'

alias tracelog='sudo gedit /sys/kernel/tracing/trace'
alias cyclelog="sudo trace-cmd clear && sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'"

alias installer="clear && sudo $DCA_KERNEL_DIR/bash/installer.sh && sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'"

alias server_echo='python3 $SIMPLE_SERVER_DIR/echo_server.py'
alias client_echo='python3 $SIMPLE_SERVER_DIR/echo_client.py'

alias clean_layer='sudo rm -rf /usr/lib/modules/$(uname -r)/layer4_5'

tracecopy () {
    sudo cp /sys/kernel/tracing/trace $GIT_DIR/\$1
    sudo trace-cmd clear
    sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'
}
EOT
"""

    ssh = """# allow scripting ssh commands
touch /home/vagrant/.ssh/config
cat <<EOT >>/home/vagrant/.ssh/config
Host 10.0.0.20
    StrictHostKeyChecking no

Host 10.0.0.30
    StrictHostKeyChecking no
EOT
"""

    netplan = """
# fix network interface GW and DNS server
cat <<EOT >>/etc/netplan/50-vagrant.yaml
      gateway4: 10.0.0.20
      nameservers:
          search: [mydomain, otherdomain]
          addresses: [10.0.0.20, 8.8.8.8]
EOT

netplan apply
"""

    dos_unix = """
# Added safety to ensure the Windows to Linux carriage return/line feed issue doesn't impact install
# This solves an old situation and prevents future situations

sudo apt install -y dos2unix

# This solution is from https://stackoverflow.com/questions/9612090/how-to-loop-through-file-names-returned-by-find
find . -name "*.sh" -exec dos2unix {} \;
"""

    with open("setup.sh", "w") as file:
        file.write(
            f"""#!/bin/bash
{ntp_setup}
{standard_params}
{aliases}
"""
        )

    if vm_type == 1:
        with open("setup.sh", "a") as file:
            file.write(
                f"""
{dns}
{ssh}
{dos_unix}
#turn swap memory back on
swapon -a
"""
            )

    with open("setup.sh", "a") as file:
        file.write(
            f"""
# finish with Layer 4.5 install script
$DCA_KERNEL_DIR/bash/installer.sh
"""
        )


def vagrant_up():
    """Prompt user and execute vagrant up."""

    # Ask the user if they want to proceed with 'vagrant up'
    proceed = input("Do you want to proceed with 'vagrant up'? (y/n): ").lower()

    if proceed != "y":
        print("Not running vagrant up.")
        sys.exit()

    else:
        print("*** Initiating Vagrant UP ***")
        try:
            subprocess.run(["vagrant", "up"], check=True)

        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")


if __name__ == "__main__":
    vm_type = generate_Vagrantfile()
    print("Vagrantfile Generated")
    generate_setup_sh(vm_type)
    print("Setup.sh Generated")
    vagrant_up()
