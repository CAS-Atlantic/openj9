#!/bin/bash
# Author: Aaron Graham <aaron.graham@unb.ca>, <aarongraham9@gmail.com>

pkgver=3.1
_debrel='+dfsg-8'
CARCH="amd64"
DEB_ARCHIVE="qemu-user-static_${pkgver}${_debrel}_${CARCH}.deb"
DEB_SRC="http://ftp.debian.org/debian/pool/main/q/qemu/"

echo -e "${0##*/}@${HOSTNAME}: Init:\n"

echo -e "${0##*/}@${HOSTNAME}: wget $DEB_SRC -O data.tar.xz:\n"

# Get 'http://ftp.debian.org/debian/pool/main/q/qemu/qemu-user-static-${pkgver}${_debrel}_${CARCH}.deb' as 'data.tar.xz': (e.g. 'qemu-user-static_3.1+dfsg-8_arm64.deb'):
wget $DEB_SRC$DEB_ARCHIVE

echo -e "${0##*/}@${HOSTNAME}: sudo apt install ./$DEB_ARCHIVE:\n"
echo -e "${0##*/}@${HOSTNAME}: Please Provide Your Sudo Password for QEMU APT Install:\n"

sudo apt install ./$DEB_ARCHIVE

echo -e "${0##*/}@${HOSTNAME}: End.\n"

