# VIRTUALBOX & VmWare

f'''
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

  
  # Single machine for running sample modules
  # config.vm.box =  "squanchy/layer4.5"
'''

# Virtualbox Server/client
f'''
  # Multiple machines for running experiment modules
  config.vm.define "{server/client}" do |{server/client}|
    {server/client}.vm.box = "squanchy/layer4.5"
    {server/client}.vm.box_version = "{box_version}"
    {server/client}.vm.network "private_network", ip: "{ip_addr}", {vbox_spec = virtualbox__intnet: "layer", nic_type: "virtio"}
    {server/client}.vm.provider "{virtualbox/vmware_fusion}" do |vb|
      # Display the VirtualBox GUI when booting the machine
      {vmware_spec = vb.linked_clone = false}
      vb.gui = true
      vb.memory = 8192
      vb.cpus = 2
      
      #vm_custom: Virtualbox
      {vb.customize ['modifyvm', :id, '--clipboard', 'bidirectional']
      vb.customize ["bandwidthctl", :id, "add", "VagrantLimit", "--type", "network", "--limit", "1000m"]
      vb.customize ["modifyvm", :id, "--nicbandwidthgroup2", "VagrantLimit"]
      #vram setting helps windows 11 machines without necessary processing power
      vb.customize ["modifyvm", :id, "--vram", "128"]
      vb.customize ["modifyvm", :id, "--accelerate3d", "on"]}
    end
    
    #vm_custom: VmWare NOTE: do we need this if the bottom config avail?
    {server/client}{.vm.synced_folder "../../
    software_defined_customization", "/home/vagrant/
    software_defined_customization", owner: "vagrant",
    group: "vagrant", automount: true}
  end

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
  
  config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization"
  config.vm.synced_folder "../../software_defined_customization", "/home/vagrant/software_defined_customization", owner: "vagrant", group: "vagrant"

  config.vm.provision :shell, path: "{setup.sh/setup_vmware.sh}"
end

'''