import sys
import json
import unittest  # This is Python's built-in library for testing code. It helps you write and run tests.

# These tools let you simulate and control how parts of your code behave under test conditions.
from unittest.mock import MagicMock, patch

# Import the script you want to test. This script contains the functions that we're going to test.
sys.path.append("../")
import NCO_revoke


class TestNCORevoke(unittest.TestCase):
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
        self.mod_id = "example_mod_id"
        self.module_name = "example_module_name"
        self.revoked_list = [{"ID": self.mod_id, "ts": "timestamp"}]

    @patch("NCO_revoke.cfg")
    @patch("NCO_revoke.delete_deployed")
    @patch("NCO_revoke.insert_revoked")
    def test_handle_revoke_update(
        self, mock_insert_revoked, mock_delete_deployed, mock_cfg
    ):
        """
        Tests the handle_revoke_update function to make sure it correctly calls the delete_deployed and insert_revoked functions with the correct arguments.
        We simulate the configuration settings and the behavior of the delete and insert functions to test our function in isolation.
        """
        mock_cfg.DB_ERROR = "DB_ERROR"  # Simulate a database error configuration for the test environment.

        # Run the function under test with the test data.
        NCO_revoke.handle_revoke_update(
            self.db_connection, self.host_id, self.revoked_list
        )

        # Verify that the delete_deployed and insert_revoked functions were called correctly with our test data.
        mock_delete_deployed.assert_called_once_with(
            self.db_connection, self.host_id, self.mod_id
        )
        mock_insert_revoked.assert_called_once_with(
            self.db_connection, self.host_id, self.mod_id, "timestamp"
        )

    @patch("NCO_revoke.select_all_req_revocation")
    def test_retrieve_revoke_list_direct_return(self, mock_select_all_req_revocation):
        """
        Tests if retrieve_revoke_list correctly processes and returns the expected data when given a specific database response.
        We simulate the database response and compare the function's output to our expected output.
        """
        expected_mod_id = [self.mod_id]
        expected_mod_name = [self.module_name]
        mock_select_all_req_revocation.return_value = [
            (self.host_id, expected_mod_id[0], expected_mod_name[0])
        ]

        # Run the function and capture its output.
        actual_mod_id, actual_mod_name = NCO_revoke.retrieve_revoke_list(
            self.db_connection, self.host_id
        )

        # Check that the output matches what we expect.
        self.assertEqual(
            (actual_mod_id, actual_mod_name), (expected_mod_id, expected_mod_name)
        )

    @patch("NCO_revoke.cfg")
    @patch("NCO_revoke.select_built_module_by_id")
    @patch("NCO_revoke.delete_req_revocation_by_id")
    def test_revoke_module(
        self, mock_delete_req_revocation_by_id, mock_select_built_module_by_id, mock_cfg
    ):
        """
        Tests the revoke_module function to see if it behaves correctly when simulating a network interaction and database operation.
        This test also checks how the function deals with different simulated responses from a device.
        """
        # Configuration mocks simulate responses for different scenarios, such as errors or configurations like buffer sizes.
        mock_cfg.DB_ERROR = "DB_ERROR"
        mock_cfg.MAX_BUFFER_SIZE = 1024
        mock_cfg.REVOKE_ERROR = "REVOKE_ERROR"
        mock_select_built_module_by_id.return_value = {"module": self.module_name}
        self.conn_socket.recv.return_value = (
            b"success"  # Simulate receiving a success response from the network.
        )

        # Run the function and check that it performs the expected actions, like sending data over the network.
        result = NCO_revoke.revoke_module(
            self.conn_socket, self.db_connection, self.host_id, self.mod_id
        )
        self.conn_socket.sendall.assert_called()  # Verify network send operation was called.
        self.assertEqual(result, 0)  # Check the function returns 0 for success.

    @patch("NCO_revoke.select_all_req_deprecate")
    @patch("NCO_revoke.cfg", DB_ERROR="DB_ERROR")
    def test_retrieve_deprecated_list(self, mock_cfg, mock_select_all_req_deprecate):
        """
        Tests whether retrieve_deprecated_list correctly retrieves a list of module IDs marked for deprecation.
        It checks for correct operation under normal conditions and how it handles simulated database errors.
        """
        # Simulate a scenario where the database returns a list of deprecated module IDs.
        expected_mod_ids = [self.mod_id]
        mock_select_all_req_deprecate.return_value = [(None, self.mod_id)]

        # Check if the function returns the expected list of IDs when database operation is successful.
        result = NCO_revoke.retrieve_deprecated_list(self.db_connection, self.host_id)
        self.assertEqual(
            result, expected_mod_ids
        )  # The result should match the expected list of module IDs.

        # Simulate a database error scenario by making the mocked function return a database error.
        mock_select_all_req_deprecate.return_value = mock_cfg.DB_ERROR

        # Check how the function handles the simulated database error.
        result = NCO_revoke.retrieve_deprecated_list(self.db_connection, self.host_id)
        self.assertEqual(
            result, -1
        )  # Expect the function to return -1 to indicate a failure due to a database error.

    @patch("NCO_revoke.cfg", MAX_BUFFER_SIZE=1024, REVOKE_ERROR="REVOKE_ERROR")
    def test_deprecate_module(self, mock_cfg):
        """
        Tests deprecate_module to ensure it correctly processes responses from a device during the deprecation process.
        This test covers scenarios including a successful deprecation, a device error, and receiving invalid JSON data.
        """
        # Simulate a successful response from the device for the deprecation request.
        self.conn_socket.recv.return_value = json.dumps(
            {"ID": self.mod_id, "Result": 1}
        ).encode("utf-8")
        result = NCO_revoke.deprecate_module(
            self.conn_socket, self.db_connection, self.host_id, self.mod_id
        )
        self.assertEqual(result, 0)  # A successful deprecation should return 0.

        # Simulate a device error response for the deprecation request.
        self.conn_socket.recv.return_value = json.dumps(
            {"ID": self.mod_id, "Result": 0}
        ).encode("utf-8")
        result = NCO_revoke.deprecate_module(
            self.conn_socket, self.db_connection, self.host_id, self.mod_id
        )
        self.assertEqual(
            result, mock_cfg.REVOKE_ERROR
        )  # Expect the function to return the REVOKE_ERROR configuration in case of an error.

        # Simulate receiving invalid JSON data, which could happen if there's a communication issue.
        self.conn_socket.recv.return_value = b"invalid JSON"
        result = NCO_revoke.deprecate_module(
            self.conn_socket, self.db_connection, self.host_id, self.mod_id
        )
        # If the function has no specific handling for invalid JSON, we expect it to return None.
        self.assertIsNone(result)


if __name__ == "__main__":
    unittest.main()
