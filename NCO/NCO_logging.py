#!/usr/bin/env python3

import logging
from logging import handlers
from time import sleep

import cfg


def logger_configurer():
    root = logging.getLogger()
    file_handler = handlers.RotatingFileHandler(
        cfg.log_file, 'a', cfg.log_size, cfg.log_max)
    file_formatter = logging.Formatter('%(asctime)s  %(name)s  %(message)s')
    file_handler.setFormatter(file_formatter)
    root.addHandler(file_handler)
    root.setLevel(logging.DEBUG)

    if cfg.log_console:
        console_handler = logging.StreamHandler()
        console_formatter = logging.Formatter('%(name)s  %(message)s')
        console_handler.setFormatter(console_formatter)
        root.addHandler(console_handler)


def logger_process(exit_event, queue):
    logger_configurer()
    while True:
        while not queue.empty():
            record = queue.get()
            logger = logging.getLogger(record.name)
            logger.handle(record)
        sleep(1)
        if exit_event.is_set():
            break
