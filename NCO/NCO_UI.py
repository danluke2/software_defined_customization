import sys
import cfg
import subprocess
from CIB_helper import *


# from some_python_file import (
#     function_for_cpcon_1,
#     function_for_cpcon_2,
#     function_for_cpcon_3,
#     function_for_cpcon_4,
#     function_for_cpcon_5,
# )  # Import Python functions as needed

# Mapping of CPCON levels to corresponding shell scripts
CPCON_SCRIPTS = {
    1: "CPCON_scripts/cpcon_level_1.sh",
    2: "CPCON_scripts/cpcon_level_2.sh",
    3: "CPCON_scripts/cpcon_level_3.sh",
    4: "None",
    5: "None",
}

# # Mapping of CPCON levels to corresponding Python functions
# CPCON_FUNCTIONS = {
#     1: function_for_cpcon_1,
#     2: function_for_cpcon_2,
#     3: select_all_hosts(db_connection),
#     4: None,
#     5: None,
# }


def list_hosts(db_connection):
    """List all hosts in the database."""
    hosts = select_all_hosts(db_connection)
    print("Hosts in database:\n")
    print(f"{'Host ID':<10}{'Host IP':<15}")
    for host in hosts:
        print(f"{host[1]:<10}{host[2]:<15}")


def view_policy(db_connection):
    """View all policies in the database."""
    policies = view_all_policies(db_connection)
    if policies == None:
        print("No policies found in the database.")
        return
    elif policies == DB_ERROR:
        print("Error: Unable to retrieve policies from the database.")
        return

    print("Policies in database:")
    for policy in policies:
        print(f"Policy ID: {policy[0]}, Policy Name: {policy[1]}")


def view_alerts(db_connection):
    """View all alerts in the database."""
    alerts = select_all_alerts(db_connection)
    if alerts is None:
        print("No alerts found in the database.")
        return
    elif alerts == DB_ERROR:
        print("Error: Unable to retrieve alerts from the database.")
        return

    print("Alerts in database:\n")
    print(f"{'Host ID':<10}{'Host Alerts':<15}")
    for host_id, alert_list in alerts.items():
        for alert in alert_list:
            print(f"{host_id:<10}{alert:<15}")


def main():
    try:
        # Establish the database connection once at the beginning
        db_connection = db_connect(cfg.nco_dir + "cib.db")
    except Exception as e:
        print(f"Error: Unable to connect to the database: {e}")
        sys.exit()

    while True:
        try:
            # Prompt the user for input
            print(
                "\n Enter desired CPCON level (1-5),\n 'list' to view hosts,\n 'view' to view policies,\n 'events' to view alerts,\n or type 'exit' to quit: ",
                end="",
            )
            user_input = input().strip()

            # Check if the user wants to view policies
            if user_input.lower() == "view":
                view_policy(db_connection)
                continue

            # Check if the user wants to list hosts
            if user_input.lower() == "list":
                list_hosts(db_connection)
                continue

            if user_input.lower() == "events":
                # Call the function to view alerts (assuming it's defined elsewhere)
                view_alerts(db_connection)
                continue

            # Check if the user wants to exit
            if user_input.lower() == "exit":
                print("Exiting program.")
                sys.exit()

            # Validate the input
            if user_input.isdigit():
                user_CPCON = int(user_input)

                if 1 <= user_CPCON <= 5:
                    print(f"Setting CPCON level: {user_CPCON}")

                    # Get the corresponding script for the CPCON level
                    script_to_run = CPCON_SCRIPTS.get(user_CPCON)

                    # Run the script using subprocess
                    if script_to_run:
                        try:
                            subprocess.run(["/bin/bash", script_to_run], check=True)
                        except FileNotFoundError:
                            print(f"Error: Script {script_to_run} not found.")
                        except subprocess.CalledProcessError as e:
                            print(
                                f"Error: Script {script_to_run} failed with error: {e}"
                            )
                else:
                    print("Invalid input. Please enter an integer between 1 and 5.")
            else:
                print(
                    "Invalid input. Please enter an integer between 1 and 5 or type 'exit' to quit."
                )

        except KeyboardInterrupt:
            # Handle program exit gracefully
            print("\nExiting program.")
            sys.exit()
        except Exception as e:
            print(f"An unexpected error occurred: {e}")


if __name__ == "__main__":
    main()
