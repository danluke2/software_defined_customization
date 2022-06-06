#!/usr/bin/env python3

import socket
import os
import json
import time
import random
import threading
import signal

import cfg
from CIB_helper import *

import logging
import sys


logger = logging.getLogger(__name__)  # use module name


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
                deployed_inverse = select_deploy_inverse_by_ip(
                    db_connection, ip)
                temp = [x[0] for x in deployed_inverse]
                if inverse_list[i] in temp:
                    continue
                else:
                    insert_deploy_inverse(
                        db_connection, inverse_list[i], ip, 1, 0)


def handle_middlebox_insert(db_connection, mac, ip, port, kernel_release, type, interval):
    max_tries = 10
    counter = 0
    # repeatedly generate middlebox ID and insert into db until successful
    while counter <= max_tries:
        generated_id = random.randint(1, 65535)
        logger.info(f"Inserting middlebox {mac}, ID = {generated_id}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_middlebox(
            db_connection, mac, generated_id, ip, port, type, kernel_release, interval)
        if (err == cfg.DB_ERROR):
            logger.info("Could not insert middlebox, try again")
            if counter == max_tries:
                return cfg.DB_ERROR
            counter += 1
            continue
        else:
            break
    middlebox = select_middlebox(db_connection, mac)
    if (middlebox == cfg.DB_ERROR):
        logger.info("Could not retrieve middlebox after insert, DB_ERROR")
    return middlebox


def send_inverse_module(conn_socket, module):
    result = cfg.MIDDLEBOX_ERROR
    logger.info(f"sending inverse module {module}")
    filesize = os.path.getsize(cfg.nco_dir + cfg.inverse_dir + module)
    command = {"cmd": "recv_inverse", "name": module, "size": filesize}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    data = conn_socket.recv(1024)
    data = data.decode("utf-8")
    if data != 'Clear to send':
        logger.info(f"Middlebox can't accept, data={data}")
    else:
        with open(cfg.nco_dir + cfg.inverse_dir + module, 'rb') as file_to_send:
            logger.info(f"{module} file open")
            for data in file_to_send:
                # logger.info("sending module")
                conn_socket.sendall(data)
            logger.info(f"{module} file closed")
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
        # Device immediately sends a customization report upon connection
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(
            f"Error on initial recv call, terminating connection\n {e}")
        conn_socket.close()
        return

    device_mac = json_data["mac"]
    kernel_release = json_data["release"]
    type = json_data["type"]

    middlebox = select_middlebox(db_connection, device_mac)

    if middlebox == None:
        middlebox = handle_middlebox_insert(
            db_connection, device_mac, ip, port, kernel_release, type, interval)

    if middlebox == cfg.DB_ERROR:
        logger.info("Middlebox DB error occurred, terminating connection")
        conn_socket.close()
        return

    # update middlebox IP and Port if necessary
    if middlebox["ip"] != ip:
        err = update_middlebox(db_connection, device_mac, "ip", ip)
        if err == cfg.DB_ERROR:
            logger.info(f"Middlebox IP not updated for mac = {device_mac}")
    if middlebox["port"] != port:
        err = update_middlebox(db_connection, device_mac, "port", port)
        if err == cfg.DB_ERROR:
            logger.info(f"Middlebox Port not updated for mac = {device_mac}")

    while True:
        if exit_event_mid.is_set():
            break
        with cv:
            # wait until interval over or notified by a device thread that
            # an inverse module needs to be deployed
            cv.wait(interval)
        inverse_list = get_inverse_list(db_connection, ip)
        logger.info(f"Sending inverse {inverse_list} to ip {ip}")
        for i in range(len(inverse_list)):
            ts = send_inverse_module(conn_socket, inverse_list[i])
            if ts == cfg.MIDDLEBOX_ERROR:
                update_inverse_module_installed_status(
                    db_connection, inverse_list[i], ip, 1, -1)
            else:
                update_inverse_module_installed_status(
                    db_connection, inverse_list[i], ip, 0, ts)

    conn_socket.close()


def middlebox_process(exit_event, cv, interval, queue):
    exit_event_mid = threading.Event()
    logger.info("Middlebox process running")
    middle_threads = []

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            # timeout for listening to check if shutdown condition reached
            s.settimeout(5)
            s.bind((cfg.HOST, cfg.MIDDLE_PORT))
            logger.info('Socket bind complete')
        except socket.error as msg:
            logger.info('Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()

        s.listen()

        while True:
            if exit_event.is_set():
                exit_event_mid.set()
                logger.info("Joining middle threads")
                for x in middle_threads:
                    x.join()
                break
            try:
                (conn, (ip, port)) = s.accept()
                logger.info(f"Accepting middlebox connection from {ip}:{port}")
                middlebox = threading.Thread(target=middlebox_thread, args=(
                    conn, ip, port, cv, cfg.MAX_BUFFER_SIZE, interval, exit_event_mid))
                middlebox.start()
                middle_threads.append(middlebox)
            except socket.timeout:
                pass
            except Exception as e:
                logger.info(f"Middlebox thread creation error: {e}")
                # traceback.print_exc()

        s.close()

    logger.info("Middlebox process exiting")
