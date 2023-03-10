# This example code is in the Public Domain (or CC0 licensed, at your option.)

# Unless required by applicable law or agreed to in writing, this
# software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.

# -*- coding: utf-8 -*-

from __future__ import print_function, unicode_literals

import os
import re
import socket
import sys
from builtins import input
from threading import Event, Thread

import ttfw_idf
from common_test_methods import (get_env_config_variable, get_host_ip4_by_dest_ip, get_host_ip6_by_dest_ip,
                                 get_my_interface_by_dest_ip)

# -----------  Config  ----------
PORT = 3333
# -------------------------------


class TcpServer:

    def __init__(self, port, family_addr, persist=False):
        self.port = port
        self.socket = socket.socket(family_addr, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.settimeout(60.0)
        self.shutdown = Event()
        self.persist = persist
        self.family_addr = family_addr

    def __enter__(self):
        try:
            self.socket.bind(('', self.port))
        except socket.error as e:
            print('Bind failed:{}'.format(e))
            raise
        self.socket.listen(1)

        print('Starting server on port={} family_addr={}'.format(self.port, self.family_addr))
        self.server_thread = Thread(target=self.run_server)
        self.server_thread.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.persist:
            sock = socket.socket(self.family_addr, socket.SOCK_STREAM)
            sock.connect(('localhost', self.port))
            sock.sendall(b'Stop', )
            sock.close()
            self.shutdown.set()
        self.shutdown.set()
        self.server_thread.join()
        self.socket.close()

    def run_server(self):
        while not self.shutdown.is_set():
            try:
                conn, address = self.socket.accept()  # accept new connection
                print('Connection from: {}'.format(address))
                conn.setblocking(1)
                data = conn.recv(1024)
                if not data:
                    return
                data = data.decode()
                print('Received data: ' + data)
                reply = 'OK: ' + data
                conn.send(reply.encode())
                conn.close()
            except socket.error as e:
                print('Running server failed:{}'.format(e))
                raise
            if not self.persist:
                break


@ttfw_idf.idf_example_test(env_tag='wifi_router')
def test_examples_protocol_socket_tcpclient(env, extra_data):
    """
    steps:
      1. join AP
      2. have the board connect to the server
      3. send and receive data
    """
    dut1 = env.get_dut('tcp_client', 'examples/protocols/sockets/tcp_client', dut_class=ttfw_idf.ESP32DUT)
    # check and log bin size
    binary_file = os.path.join(dut1.app.binary_path, 'tcp_client.bin')
    bin_size = os.path.getsize(binary_file)
    ttfw_idf.log_performance('tcp_client_bin_size', '{}KB'.format(bin_size // 1024))

    # start test
    dut1.start_app()
    if dut1.app.get_sdkconfig_config_value('CONFIG_EXAMPLE_WIFI_SSID_PWD_FROM_STDIN'):
        dut1.expect('Please input ssid password:')
        env_name = 'wifi_router'
        ap_ssid = get_env_config_variable(env_name, 'ap_ssid')
        ap_password = get_env_config_variable(env_name, 'ap_password')
        dut1.write(f'{ap_ssid} {ap_password}')

    ipv4 = dut1.expect(re.compile(r'IPv4 address: (\d+\.\d+\.\d+\.\d+)[^\d]'), timeout=30)[0]
    ipv6_r = r':'.join((r'[0-9a-fA-F]{4}',) * 8)    # expect all 8 octets from IPv6 (assumes it's printed in the long form)
    ipv6 = dut1.expect(re.compile(r' IPv6 address: ({})'.format(ipv6_r)), timeout=30)[0]
    print('Connected with IPv4={} and IPv6={}'.format(ipv4, ipv6))

    my_interface = get_my_interface_by_dest_ip(ipv4)
    # test IPv4
    with TcpServer(PORT, socket.AF_INET):
        server_ip = get_host_ip4_by_dest_ip(ipv4)
        print('Connect tcp client to server IP={}'.format(server_ip))
        dut1.write(server_ip)
        dut1.expect(re.compile(r'OK: Message from ESP32'))
    # test IPv6
    with TcpServer(PORT, socket.AF_INET6):
        server_ip = get_host_ip6_by_dest_ip(ipv6, my_interface)
        print('Connect tcp client to server IP={}'.format(server_ip))
        dut1.write(server_ip)
        dut1.expect(re.compile(r'OK: Message from ESP32'))


if __name__ == '__main__':
    if sys.argv[1:] and sys.argv[1].startswith('IPv'):     # if additional arguments provided:
        # Usage: example_test.py <IPv4|IPv6>
        family_addr = socket.AF_INET6 if sys.argv[1] == 'IPv6' else socket.AF_INET
        with TcpServer(PORT, family_addr, persist=True) as s:
            print(input('Press Enter stop the server...'))
    else:
        test_examples_protocol_socket_tcpclient()
