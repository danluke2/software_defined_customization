import subprocess


def get_disabled_interfaces():
    """Get a list of network interfaces that are currently down"""
    try:
        result = subprocess.run(
            ["ip", "link"], capture_output=True, text=True, check=True
        )
        interfaces = []
        lines = result.stdout.split("\n")

        for line in lines:
            if "state DOWN" in line:
                iface = line.split(":")[1].strip()
                interfaces.append(iface)

        return interfaces

    except subprocess.CalledProcessError as e:
        print(f"Error retrieving network interfaces: {e}")
        return []


def restore_network():
    """Bring up all network interfaces that are currently down"""
    interfaces = get_disabled_interfaces()

    if not interfaces:
        print("No disabled network interfaces found.")
        return

    for iface in interfaces:
        try:
            print(f"Restoring network interface: {iface}")
            subprocess.run(["sudo", "ip", "link", "set", iface, "up"], check=True)
            print(f"Successfully restored {iface}")
        except subprocess.CalledProcessError as e:
            print(f"Failed to restore {iface}: {e}")


if __name__ == "__main__":
    restore_network()
