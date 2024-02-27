import os

def valid_int(val):
  if val.isnumeric():
    return val
  else:
    val = input('Error: Please provide a valid integer: ')
    valid_int(val)


# Take input variables

print('*** Vagrantfile setup for VMware ***\n')
memory = valid_int(input('Provide in integer the memory to allocate: '))
cpu = valid_int(input('Provide in integer the number of CPU to allocate: '))

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
  
  config.vm.provision :shell, path: "setup_vmware.sh"
end

'''

with open('Vagrantfile', 'w') as file:
    file.write(vagrantfile_content)
  


# Run vagrant up
os.system('vagrant up')