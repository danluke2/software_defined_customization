#!/usr/bin/env python3

import socket
import os
import json
import time
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Util.Padding import unpad

import logging
import sys

import cfg
from CIB_helper import *


logger = logging.getLogger(__name__)  # use module name




########################### SECURITY FUNCTION #################################


# Note: generic key generation for testing functionality only
def generate_key():
    # key is a byte string
    key = os.urandom(cfg.KEY_SIZE)
    return key



def check_challenge(db_connection, host_id):
    challenge_list = []
    now = int(time.time())

    deployed_list_rows = select_all_deployed_rows(db_connection, host_id)
    module_id_list = [x[1] for x in deployed_list_rows]
    sec_windows = [x[4] for x in deployed_list_rows]
    sec_ts = [x[5] for x in deployed_list_rows]

    for i in range(len(module_id_list)):
        if now - sec_ts[i] > sec_windows[i]:
            challenge_list.append(module_id_list[i])
    return challenge_list




def request_challenge_response(conn_socket, db_connection, host_id, mod_id, buffer_size):
    result = cfg.REVOKE_MOD
    iv = os.urandom(cfg.IV_SIZE)
    command = {"cmd": "challenge"}
    command["id"] = mod_id
    command["iv"] = iv.hex()

    data = os.urandom(8)
    temp = data.hex()
    logger.info(f"Challenge before encrypt as hex is {temp}")

    key = select_built_module_key(db_connection, host_id, mod_id)
    cipher = AES.new(key, AES.MODE_CBC, iv)
    ct_bytes = cipher.encrypt(pad(data, AES.block_size))
    ct = ct_bytes.hex()
    command["msg"] = ct

    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    logger.info(f"Sent challenge request to {host_id}, challenge: {command}")


    # process the response now
    try:
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
        iv = bytes.fromhex(json_data["IV"])
        ct = bytes.fromhex(json_data["Response"])
        cipher = AES.new(key, AES.MODE_CBC, iv)
        pt = unpad(cipher.decrypt(ct), AES.block_size)
        temp2 = pt.hex()
        logger.info(f"The decrypted response as hex is {temp2}")
        if temp2.startswith(temp):
            logger.info("Challenge string matches")
            temp2 = temp2[-6:]
            temp2 = int(temp2, 16)
            if temp2 == mod_id:
                logger.info("Module_id matches")
                result = 0
            else:
                logger.info(f"Module_id string mismatch, was {temp2} and should be {mod_id}")

    except Exception as e:
        logger.info(f"Error processing challenge response: {e}")
        result = cfg.REVOKE_MOD

    return result
