import os
import subprocess

def valid_int(val):
  ''' Verify input is an integer type.'''
  
  if val.isnumeric():
    return val
  
  else:
    valid_int(input('Error: Please provide a valid integer: '))


def machine_input(name, box_version, ip_addr, vbox_spec, provider, vmware_spec, memory, cpu):
    '''Populate and return the vagrantfile cofiguration variables for new machine. '''
    
    machines = f'''
  # Multiple machines for running experiment modules
  config.vm.define "{name}" do |server|
    {name}.vm.box = "squanchy/layer4.5"
    {name}.vm.box_version = "{box_version}"
    {name}.vm.network "private_network", ip: "{ip_addr}", {vbox_spec}
    {name}.vm.provider "{provider}" do |vb|
      # Display the VirtualBox GUI when booting the machine
      {vmware_spec}
      vb.gui = true
      vb.memory = {memory}
      vb.cpus = {cpu}'''

    return machines


def setup_file(setup_path):
    '''Populate and return the Vagrantfile setup_path for specific VM. '''
    
    setup = f'''
  config.vm.provision :shell, path: "{setup_path}"
end'''
    
    return setup


def generate_Vagrantfile():
    ''' Generate a Vagrantfile. '''

    ############# STATIC VAGRANTFILE CONTENT #############
    begin = '''# -*- mode: ruby -*-
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
  # config.vm.box =  "squanchy/layer4.5"'''

    vbox_custom = '''
      vb.customize ['modifyvm', :id, '--clipboard', 'bidirectional']
      vb.customize ["bandwidthctl", :id, "add", "VagrantLimit", "--type", "network", "--limit", "1000m"]
      vb.customize ["modifyvm", :id, "--nicbandwidthgroup2", "VagrantLimit"]
      #vram setting helps windows 11 machines without necessary processing power
      vb.customize ["modifyvm", :id, "--vram", "128"]
      vb.customize ["modifyvm", :id, "--accelerate3d", "on"]
    end
  end'''

    vmware_custom = '''
    end
    nps.vm.synced_folder "../../
    software_defined_customization", "/home/vagrant/
    software_defined_customization", owner: "vagrant",
    group: "vagrant", automount: true
  end'''
    
    comments = '''
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
  # SHELL'''
    
    
    ############# INQUIRY VARIABLES #############
    vm_type = int(valid_int(input("1) Virtualbox \n2) VmWare \nSelect your virtual machine option above (1 or 2): ")))
    vm_num = int(valid_int(input("How many machines would you like to activate?: ")))
    
    memory = valid_int(input('Enter memory value (MB) in integer (Recommend 8192): '))
    cpu = valid_int(input('Enter CPU count in integer (Recommend 2):'))

    
    ############# VAGRANTFILE GENERATION #############
        
    # Variables tailored to vm_type
    if vm_type == 1:
        provider = "virtualbox"
        box_version = "1.2"
        vbox_spec = '''virtualbox__intnet: "layer", nic_type: "virtio" '''
        vmware_spec = ""      # Not required for vbox
        setup_path = "setup.sh"
        # file_path = os.path.join(os.getcwd(), "vagrant", "Vagrantfile")
        vm_num += 1
                
    elif vm_type == 2:
        provider = "vmware_fusion"
        box_version = "1.3"
        vbox_spec = ""      # Not required for vmware
        vmware_spec = "vb.linked_clone = false"
        setup_path = "setup_vmware.sh"
        # file_path = os.path.join(os.getcwd(), "vagrant_vmware", "Vagrantfile")
        
    # file_path = os.path.join(os.getcwd(), "vagrant_test", "Vagrantfile")
    file_path = "Vagrantfile"
    
    # Vagrantfile beginning
    with open(file_path, 'w') as file:
        file.write (begin)


    # Appending number of machines
    for i in range(vm_num):
        # Update IP Address
        ip_addr = "10.0.0." + str(20 + i)
        
        # Update name for each machine
        if i == 0:
            name = "server"
        else: 
            name = "client_" + str(i)
        
        #Appending machine config
        with open(file_path, 'a') as file:
            file.write(machine_input(name, box_version, ip_addr, vbox_spec, provider, vmware_spec, memory, cpu))
        
        if vm_type == 1:
            with open (file_path, 'a') as file:
                file.write(vbox_custom)
                
        elif vm_type == 2:
            with open(file_path, 'a') as file:
                file.write(vmware_custom)
                
    # Appending the comments of Vagrantfile
    with open(file_path, 'a') as file:
        file.write(comments)
    
    if vm_type == 1:
        with open(file_path, 'a') as file:
            file.write('''  config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization"''')
        # config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization"
        # config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization", owner: "vagrant", group: "vagrant"

    # Appending setup.sh path            
    with open(file_path, 'a') as file:
        file.write(setup_file(setup_path))
        
        
def vagrant_up():
    '''Prompt user and execute vagrant up. '''
    
    # Ask the user if they want to proceed with 'vagrant up'
    proceed = input("Do you want to proceed with 'vagrant up'? (y/n): ").lower()
    
    if proceed != 'y':
        print("Not running vagrant up.")
        exit
        
    else:
        # subprocess.run (['cd', 'vagrant_test'], check = True)
        print("*** Initiating Vagrant UP ***")
        try:
            subprocess.run(['vagrant', 'up'], check = True)
            
        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")


if __name__ == "__main__":
    generate_Vagrantfile()
    print('Vagrantfile Generated')
    vagrant_up()
    
