import subprocess

def main():
    
    try:
        os_type = int(input("Select your OS option below (1-3): \n 1) Windows \n 2) Mac Intel \n 3) Mac ARM \n"))

    except:
        print("Must enter integer from 1 to 3.")
        main()
        
    if os_type == 1 or os_type == 2:
        try:
            subprocess.run(['python', 'vagrant/setup_vagrantfile_keo.py'], check = True)
        
        except subprocess.CalledProcessError:
            print('Enter command in terminal: python vagrant/setup_vagrantfile_keo.py')
            
            
    elif os_type == 3:
        try: 
            subprocess.run(['python3',  'vagrant_vmware/dang_test.py'], check = True)

        except subprocess.CalledProcessError:
            print('Enter command in terminal: python3 vagrant_vmware/dang_test.py')
            
    else:
        main()

if __name__ == "__main__":
    main()