#!/bin/sh

for i in `cat first_octets.txt`; do 
    echo -n "$i: "
    curl http://ip-to-country.webhosting.info/node/view/36?ip_address=$i 2>/dev/null | grep "belongs to" | sed "s/.*<b>//" | sed "s/<\/b>.*//"
done