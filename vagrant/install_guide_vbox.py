import subprocess

def update_vagrantfile(memory, cpus):
    vagrantfile_path = "Vagrantfile"
    try:
        with open(vagrantfile_path, 'r') as file:
            lines = file.readlines()

        # Find the indentation level
        indentation = ""
        for line in lines:
            if "vb.memory" in line or "vb.cpus" in line:
                indentation = line[:line.find("vb")]
                break

        # Find and replace lines containing memory and CPU settings
        for i in range(len(lines)):
            # Check if the line is not a comment
            if not lines[i].strip().startswith("#"):
                if "vb.memory" in lines[i]:
                    lines[i] = f'{indentation}vb.memory = {memory}\n'
                elif "vb.cpus" in lines[i]:
                    lines[i] = f'{indentation}vb.cpus = {cpus}\n'

        # Write the updated lines back to the Vagrantfile
        with open(vagrantfile_path, 'w') as file:
            file.writelines(lines)

        print("Vagrantfile updated successfully!")
        return True
    except FileNotFoundError:
        print("Vagrantfile not found. Make sure you're running this script in the directory containing Vagrantfile.")
        return False


def main():
    memory = input("Enter memory value (MB) (Recommend 8192): ")
    cpus = input("Enter CPU count (Recommend 2): ")

    # Validate memory and cpus inputs
    try:
        memory = int(memory)
        cpus = int(cpus)
    except ValueError:
        print("Memory and CPU count must be integers.")
        return
    
    # Ask the user if they want to proceed with 'vagrant up'
    proceed = input("Do you want to proceed with 'vagrant up'? (y/n): ").lower()
    if proceed != 'y':
        print("Aborting vagrant up.")
        return

    if update_vagrantfile(memory, cpus):
        # Run 'vagrant up' command
        try:
            subprocess.run(["vagrant", "up"], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")
    else:
        print("Failed to update Vagrantfile. Aborting vagrant up command.")


if __name__ == "__main__":
    main()
