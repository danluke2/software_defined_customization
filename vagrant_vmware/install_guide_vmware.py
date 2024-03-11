import subprocess

def valid_int(val):
  ''' Verify input is an integer type.'''
  
  if val.isnumeric():
    return val
  
  else:
    val = input('Error: Please provide a valid integer: ')
    valid_int(val)


# Take input variables
def main():
  '''Prompt users for required inputs to generate Vagrantfile.
  Then run vagrant up.'''
  
  print('*** Vagrantfile setup for VMware ***\n')
  
  # Prompt for inputs
  memory = valid_int(input('Provide in integer the memory to allocate: '))
  cpu = valid_int(input('Provide in integer the number of CPU to allocate: '))

  # Vagrantfile details
  vagrantfile_content = f'''
  # -*- mode: ruby -*-
  # vi: set ft=ruby :

  # All Vagrant configuration is done below. The "2" in Vagrant.configure
  # configures the configuration version (we support older styles for
  # backwards compatibility). Please don't change it unless you know what
  # you're doing.
  Vagrant.configure("2") do |config|
    # The most common configuration options are documented and commented below.
    # For a complete reference, please see the online documentation at
    # https://docs.vagrantup.com.



    # Single ARM Based VM for VMWare
    config.vm.define "nps" do |nps|
      nps.vm.box = "squanchy/layer4.5"
      nps.vm.box_version = "1.3"
      nps.vm.provider "vmware_fusion" do |vb|
        # this prevents a file open error when starting up likely caused by 
        # the way I created the vagrant box
        vb.linked_clone = false
        # Display the GUI when booting the machine
        vb.gui = true
        vb.memory = {memory}
        vb.cpus = {cpu}
      end
    end


    # TODO: Should we set the network up also?
    # nps.vm.network "public_network", adapter: 0, auto_config: false

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
    config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization", owner: "vagrant", group: "vagrant"

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
    config.vm.provision :shell, path: "setup_vmware.sh"
  end

  '''

  # Generate Vagrantfile
  with open('Vagrantfile', 'w') as file:
      file.write(vagrantfile_content)
    

  # Prompt for vagrant up
  proceed = input("Do you want to proceed with 'vagrant up'? (y/n): ").lower()
  if proceed != 'y':
    print("Aborting vagrant up.")
    exit
    
  else:  
    # Run vagrant up
    subprocess.run(['vagrant', 'up'], check = True)


if __name__ == "__main__":
    main()