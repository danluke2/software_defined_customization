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


def main():
    while True:
        try:
            # Prompt the user for input
            print("Enter desired CPCON level (1-5) or type 'exit' to quit: ", end="")
            user_input = input().strip()

            # Check if the user wants to exit
            if user_input.lower() == "exit":
                print("Exiting program.")
                sys.exit()

            # Validate the input
            if user_input.isdigit():
                user_CPCON = int(user_input)

                if 1 <= user_CPCON <= 5:
                    print(f"Setting CPCON level: {user_CPCON}")

                    # Get the corresponding script and function for the CPCON level
                    script_to_run = CPCON_SCRIPTS.get(user_CPCON)
                    # function_to_call = CPCON_FUNCTIONS.get(user_CPCON)

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

                    # db_connection = db_connect(cfg.nco_dir + "cib.db")

                    # Call the Python function
                    # if function_to_call:
                    #     try:
                    #         function_to_call()
                    #     except Exception as e:
                    #         print(f"Error: Failed to execute Python function: {e}")

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


if __name__ == "__main__":
    main()
