import sys


def main():
    user_CPCON = None  # Initialize the CPCON variable

    while True:
        try:
            # Prompt the user for input
            print("Enter desired CPCON level (1-5): ", end="")
            user_input = input().strip()

            # Validate the input
            if user_input.isdigit():
                user_CPCON = int(user_input)
                if 1 <= user_CPCON <= 5:
                    print(f"CPCON level set to {user_CPCON}")
                else:
                    print("Invalid input. Please enter an integer between 1 and 5.")
            else:
                print("Invalid input. Please enter an integer between 1 and 5.")

        except KeyboardInterrupt:
            # Handle program exit gracefully
            print("\nExiting program.")
            sys.exit()


if __name__ == "__main__":
    main()
