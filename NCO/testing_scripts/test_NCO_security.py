import sys
import json
import unittest


# These tools let you simulate and control how parts of your code behave under test conditions.
from unittest.mock import MagicMock, patch

# Import the script you want to test. This script contains the functions that we're going to test.
sys.path.append("../")
import NCO_security


# this test ensures that the key generated is of the correct length
class TestNCOSecurity(unittest.TestCase):
    """
    A class that contains our tests. It inherits from unittest.TestCase,
    which includes helpful methods for creating and running tests.
    Each method in this class is a test case designed to test a specific part of the NCO_revoke script.
    """

    def setUp(self):
        """
        setUp is a special method run before each test method.
        It's used to set up objects and data that will be used in the tests.
        This way, you don't have to repeat code in each test.
        """
        # Mocks are fake objects that simulate the behavior of real objects. Here, we create fake versions of a database connection and a network socket.
        self.db_connection = MagicMock()
        self.conn_socket = MagicMock()

        # Example data that will be used across multiple tests.
        self.host_id = "example_host_id"

    @patch("NCO_security.cfg")
    @patch("NCO_security.select_all_deployed_rows")
    def test_generate_key_length(self, mock_select_all_deployed_rows, mock_cfg):
        """
        Tests the generate_key function to make sure it generates a key of the correct length.
        We simulate the configuration settings and the behavior of the select_all_deployed_rows function to test our function in isolation.
        """
        mock_cfg.KEY_SIZE = (
            32  # Simulate the key size configuration for the test environment.
        )

        # Generate a key
        key = NCO_security.generate_key()

        # Check if the key length matches cfg.KEY_SIZE
        self.assertEqual(len(key), mock_cfg.KEY_SIZE)

    def test_generate_key_length_incorrect(
        self, mock_select_all_deployed_rows, mock_cfg
    ):
        """
        Tests the generate_key function to make sure it generates a key of the correct length.
        We simulate the configuration settings and the behavior of the select_all_deployed_rows function to test our function in isolation.
        """
        mock_cfg.KEY_SIZE = (
            16  # Simulate incorrect key size configuration for the test environment.
        )

        # Generate a key
        key = NCO_security.generate_key()

        # Check if the incorrect key length matches cfg.KEY_SIZE
        self.assertNotEqual(len(key), mock_cfg.KEY_SIZE)

    @patch("NCO_security.time")
    @patch("NCO_security.select_all_deployed_rows")
    def test_check_challenge(self, mock_select_all_deployed_rows, mock_time):
        """
        Tests the check_challenge function to make sure it returns the correct result.
        We simulate the behavior of the select_all_deployed_rows function to test our function in isolation.
        """
        # Mock the time.time() function to return a specific value for testing
        mock_time.time.return_value = 1234567890

        # Run the function under test with the test data.
        result = NCO_security.check_challenge(self.db_connection, self.host_id)

        # In this example, no challenges should be returned as the current time is equal to the sec_ts of the first module
        self.assertEqual(result, [])

    @patch("NCO_security.select_all_deployed_rows")
    def test_check_challenge_valid_db_response(self, mock_select_all_deployed_rows):
        """
        Test check_challenge function to verify it correctly processes a valid database response.
        """
        # Mock the database response
        # Assuming select_all_deployed_rows returns a list of tuples, each representing a row
        mock_db_response = [
            (1, "challenge1", "details1", 1234567890),
            (2, "challenge2", "details2", 1234567891),
        ]
        mock_select_all_deployed_rows.return_value = mock_db_response

        # Define what the expected outcome is based on the mocked db response
        # This will depend on your actual implementation of check_challenge
        expected_result = [
            "challenge1",
            "challenge2",
        ]  # for example, expecting a list of challenge IDs

        # Call the function under test and assert it processes the mocked response correctly
        actual_result = NCO_security.check_challenge(self.db_connection, self.host_id)
        self.assertEqual(actual_result, expected_result)

        # Additionally, verify that select_all_deployed_rows was called with the correct parameters
        mock_select_all_deployed_rows.assert_called_once_with(
            self.db_connection, self.host_id
        )


if __name__ == "__main__":
    unittest.main()
