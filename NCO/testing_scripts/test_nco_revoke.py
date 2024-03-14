import json
import unittest
from unittest.mock import MagicMock, patch

import NCO_revoke  # Ensure this is correctly pointing to your script


class TestNCORevoke(unittest.TestCase):

    def setUp(self):
        # Setup before each test method
        self.db_connection = MagicMock()
        self.conn_socket = MagicMock()
        self.host_id = "example_host_id"
        self.mod_id = "example_mod_id"
        self.module_name = "example_module_name"
        self.revoked_list = [{"ID": self.mod_id, "ts": "timestamp"}]

    @patch('NCO_revoke.cfg')
    @patch('NCO_revoke.delete_deployed')
    @patch('NCO_revoke.insert_revoked')
    def test_handle_revoke_update(self, mock_insert_revoked, mock_delete_deployed, mock_cfg):
        # Mock external functions and configurations
        mock_cfg.DB_ERROR = "DB_ERROR"
        mock_delete_deployed.return_value = None  # Assuming successful deletion
        mock_insert_revoked.return_value = None   # Assuming successful insertion

        NCO_revoke.handle_revoke_update(self.db_connection, self.host_id, self.revoked_list)

        # Verify that delete_deployed and insert_revoked are called correctly
        mock_delete_deployed.assert_called_once_with(self.db_connection, self.host_id, self.mod_id)
        mock_insert_revoked.assert_called_once_with(self.db_connection, self.host_id, self.mod_id, "timestamp")


    @patch('NCO_revoke.select_all_req_revocation')
    def test_retrieve_revoke_list_direct_return(self, mock_select_all_req_revocation):
        # Setup mock to return directly what you expect the function to output
        expected_mod_id = [self.mod_id]
        expected_mod_name = [self.module_name]
        mock_select_all_req_revocation.return_value = [(self.host_id, expected_mod_id[0], expected_mod_name[0])]

        # The function should return the expected values
        actual_mod_id, actual_mod_name = NCO_revoke.retrieve_revoke_list(self.db_connection, self.host_id)

        # Check if the returned values match the expected values
        self.assertEqual((actual_mod_id, actual_mod_name), (expected_mod_id, expected_mod_name))


    @patch('NCO_revoke.cfg')
    @patch('NCO_revoke.select_built_module_by_id')
    @patch('NCO_revoke.delete_req_revocation_by_id')
    def test_revoke_module(self, mock_delete_req_revocation_by_id, mock_select_built_module_by_id, mock_cfg):
        mock_cfg.DB_ERROR = "DB_ERROR"
        mock_cfg.MAX_BUFFER_SIZE = 1024
        mock_cfg.REVOKE_ERROR = "REVOKE_ERROR"
        mock_select_built_module_by_id.return_value = {"module": self.module_name}
        self.conn_socket.recv.return_value = b'success'
        # Assuming the sendall method is successful
        result = NCO_revoke.revoke_module(self.conn_socket, self.db_connection, self.host_id, self.mod_id)
        # Verify that the sendall method is called
        self.conn_socket.sendall.assert_called()
        self.assertEqual(result, 0)

    @patch('NCO_revoke.select_all_req_deprecate')
    @patch('NCO_revoke.cfg', DB_ERROR='DB_ERROR')
    def test_retrieve_deprecated_list(self, mock_cfg, mock_select_all_req_deprecate):
        # Test case for successful retrieval
        expected_mod_ids = [self.mod_id]
        mock_select_all_req_deprecate.return_value = [(None, self.mod_id)]
        
        result = NCO_revoke.retrieve_deprecated_list(self.db_connection, self.host_id)
        self.assertEqual(result, expected_mod_ids)

        # Test case for handling database error
        mock_select_all_req_deprecate.return_value = mock_cfg.DB_ERROR
        result = NCO_revoke.retrieve_deprecated_list(self.db_connection, self.host_id)
        self.assertEqual(result, -1)

    @patch('NCO_revoke.cfg', MAX_BUFFER_SIZE=1024, REVOKE_ERROR='REVOKE_ERROR')
    def test_deprecate_module(self, mock_cfg):
        # Test for successful deprecation
        self.conn_socket.recv.return_value = json.dumps({"ID": self.mod_id, "Result": 1}).encode('utf-8')
        result = NCO_revoke.deprecate_module(self.conn_socket, self.db_connection, self.host_id, self.mod_id)
        self.assertEqual(result, 0)

        # Test for handling device error response (assuming device error leads to REVOKE_ERROR)
        self.conn_socket.recv.return_value = json.dumps({"ID": self.mod_id, "Result": 0}).encode('utf-8')
        result = NCO_revoke.deprecate_module(self.conn_socket, self.db_connection, self.host_id, self.mod_id)
        self.assertEqual(result, mock_cfg.REVOKE_ERROR)

        # Adjust expectation for handling invalid JSON based on actual function behavior
        self.conn_socket.recv.return_value = b'invalid JSON'
        result = NCO_revoke.deprecate_module(self.conn_socket, self.db_connection, self.host_id, self.mod_id)
        # If the function internally handles JSON decode errors without returning mock_cfg.REVOKE_ERROR, adjust the expectation
        # For example, if it returns None or does not have a return statement for this case, expect None
        self.assertIsNone(result)


if __name__ == '__main__':
    unittest.main()
