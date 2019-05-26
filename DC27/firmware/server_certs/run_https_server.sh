#!/bin/bash
echo "ca_key.pem pasword is darknet, just locally generated not used anywhere else"
openssl s_server -WWW -key ca_key.pem -cert ca_cert.pem -port 8070
