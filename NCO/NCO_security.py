#!/usr/bin/env python3

import socket
import os
import json
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Util.Padding import unpad

import cfg
from CIB_helper import *




########################### SECURITY FUNCTION #################################


# Note: generic key generation for testing functionality only
def generate_key():
    # key is a byte string
    key = os.urandom(cfg.KEY_SIZE)
    return key



def request_challenge_response(conn_socket, db_connection, host_id, active_list, buffer_size):
    try:
        active_module = active_list[0]
        module_id = active_module["ID"]
    except:
        print(f"Error with active list: {active_list}")
        return


    iv = os.urandom(cfg.IV_SIZE)
    command = {"cmd": "challenge"}
    command["id"] = module_id
    command["iv"] = iv.hex()

    data = os.urandom(8)
    temp = data.hex()
    print(f"Challenge before encrypt as hex is {temp}")

    key = select_built_module_key(db_connection, host_id, module_id)
    cipher = AES.new(key, AES.MODE_CBC, iv)
    ct_bytes = cipher.encrypt(pad(data, AES.block_size))
    ct = ct_bytes.hex()
    command["msg"] = ct

    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print(f"Sent challenge request to {host_id}, challenge: {command}")


    # process the response now
    try:
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
        iv = bytes.fromhex(json_data["IV"])
        ct = bytes.fromhex(json_data["Response"])
        cipher = AES.new(key, AES.MODE_CBC, iv)
        pt = unpad(cipher.decrypt(ct), AES.block_size)
        temp2 = pt.hex()
        print(f"The decrypted response as hex is {temp2}")
        if temp2.startswith(temp):
            print("Challenge string matches")
            temp2 = temp2[-6:]
            temp2 = int(temp2, 16)
        if temp2 == module_id:
            print("Module_id matches")
        else:
            print(f"Module_id string mismatch, was {temp2} and should be {module_id}")

    except json.decoder.JSONDecodeError as e:
        print(f"Error on process report recv call, {e}")
        return cfg.CLOSE_SOCK
