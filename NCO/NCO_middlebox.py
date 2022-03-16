#!/usr/bin/env python3

import socket
import os
import json
import time
import random

import cfg
from CIB_helper import *




######################### MIDDLEBOX FUNCTIONS #############################

def update_inverse_module_requirements(db_connection, modules):
    for module in modules:
        rows = select_inverse_by_module(db_connection, module)
        inverse_list = [x[1] for x in rows]
        types = [x[2] for x in rows]
        for i in range(len(inverse_list)):
            middlebox_list = select_middlebox_by_type(db_connection, types[i])
            middle_ip_list = [x[2] for x in middlebox_list]
            for ip in middle_ip_list:
                # if inverse already in deploy list, then skip
                deployed_inverse = select_deploy_inverse_by_ip(db_connection, ip)
                temp = [x[0] for x in deployed_inverse]
                if inverse_list[i] in temp:
                    continue
                else:
                    insert_deploy_inverse(db_connection, inverse_list[i], ip, 1, 0)


def handle_middlebox_insert(db_connection, mac, ip, port, kernel_release, type, interval):
    max_tries = 10
    counter = 0
    #repeatedly generate middlebox ID and insert into db until successful
    while counter <= max_tries:
        generated_id = random.randint(1,65535)
        print(f"Inserting middlebox {mac}, ID = {generated_id}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_middlebox(db_connection, mac, generated_id, ip, port, type, kernel_release, interval)
        if (err == cfg.DB_ERROR):
            print("Could not insert middlebox, try again")
            if counter == max_tries:
                return cfg.DB_ERROR
            counter +=1
            continue
        else:
            break
    middlebox = select_middlebox(db_connection, mac)
    if (middlebox == cfg.DB_ERROR):
        print("Could not retrieve middlebox after insert, DB_ERROR")
    return middlebox


def send_inverse_module(conn_socket, module):
    result = cfg.MIDDLEBOX_ERROR
    print(f"sending inverse module {module}")
    filesize = os.path.getsize(cfg.nco_dir + cfg.inverse_dir + module)
    command = {"cmd": "recv_inverse", "name": module, "size": filesize}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    data = conn_socket.recv(1024)
    data = data.decode("utf-8")
    if data != 'Clear to send':
        print(f"Middlebox can't accept, data={data}")
    else:
        with open(cfg.nco_dir + cfg.inverse_dir + module, 'rb') as file_to_send:
            print(f"{module} file open")
            for data in file_to_send:
                # print("sending module")
                conn_socket.sendall(data)
            print(f"{module} file closed")
            result = int(time.time())
    return result



def get_inverse_list(db_connection, ip):
    inverse_to_deploy = select_deploy_inverse_by_ip(db_connection, ip)
    inverse_list = []
    for mod in inverse_to_deploy:
        if mod[2]:
            inverse_list.append(mod[0])
    return inverse_list




def middlebox_thread(conn_socket, ip, port, cv, buffer_size, interval, exit_event_mid):
    db_connection = db_connect(cfg.nco_dir + 'cib.db')
    # process initial report
    try:
        #Device immediately sends a customization report upon connection
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on initial recv call, terminating connection\n {e}")
        conn_socket.close()
        return

    device_mac = json_data["mac"]
    kernel_release = json_data["release"]
    type = json_data["type"]

    middlebox = select_middlebox(db_connection, device_mac)

    if middlebox == None:
        middlebox = handle_middlebox_insert(db_connection, device_mac, ip, port, kernel_release, type, interval)

    if middlebox == cfg.DB_ERROR:
        print("Middlebox DB error occurred, terminating connection")
        conn_socket.close()
        return

    # update middlebox IP and Port if necessary
    if middlebox["ip"] != ip:
        err = update_middlebox(db_connection, device_mac, "ip", ip)
        if err == cfg.DB_ERROR:
            print(f"Middlebox IP not updated for mac = {device_mac}")
    if middlebox["port"] != port:
        err = update_middlebox(db_connection, device_mac, "port", port)
        if err == cfg.DB_ERROR:
            print(f"Middlebox Port not updated for mac = {device_mac}")


    while True:
        if exit_event_mid.is_set():
            break
        with cv:
            # wait until interval over or notified by a device thread that
            # an inverse module needs to be deployed
            cv.wait(interval)
        inverse_list = get_inverse_list(db_connection, ip)
        print(f"Sending inverse {inverse_list} to ip {ip}")
        for i in range(len(inverse_list)):
            ts = send_inverse_module(conn_socket, inverse_list[i])
            if ts == cfg.MIDDLEBOX_ERROR:
                update_inverse_module_installed_status(db_connection, inverse_list[i], ip, 1, -1)
            else:
                update_inverse_module_installed_status(db_connection, inverse_list[i], ip, 0, ts)

    conn_socket.close()
