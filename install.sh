#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only

set -eu

package_name='yoga-s740-vpc-poll'
package_version='1.0.0'
module_name='yoga_s740_vpc_poll'
target='/usr/src/yoga-s740-vpc-poll-1.0.0'
autoload_target='/etc/modules-load.d/yoga-s740-vpc-poll.conf'
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ "$(id -u)" -ne 0 ]; then
	echo "Run this installer as root: sudo ./install.sh" >&2
	exit 1
fi

if [ "$(cat /sys/class/dmi/id/sys_vendor 2>/dev/null || true)" != 'LENOVO' ] ||
   [ "$(cat /sys/class/dmi/id/product_name 2>/dev/null || true)" != '81RS' ]; then
	echo "Refusing to install: this workaround supports only Lenovo product 81RS." >&2
	exit 1
fi

for command_name in dkms make install modprobe uname; do
	if ! command -v "$command_name" >/dev/null 2>&1; then
		echo "Missing required command: $command_name" >&2
		exit 1
	fi
done

kernel_release=$(uname -r)
if [ ! -e "/lib/modules/$kernel_release/build/Makefile" ]; then
	echo "Missing headers for $kernel_release." >&2
	echo "On Ubuntu/Debian, install: linux-headers-$kernel_release" >&2
	exit 1
fi

if [ -e "$target" ] || dkms status -m "$package_name" 2>/dev/null | grep -q .; then
	echo "Version $package_version is already present, or another version is registered." >&2
	echo "Remove the existing installation before installing this release." >&2
	exit 1
fi

install -d -m 0755 "$target"
install -m 0644 "$script_dir/yoga_s740_vpc_poll.c" "$target/yoga_s740_vpc_poll.c"
install -m 0644 "$script_dir/Makefile" "$target/Makefile"
install -m 0644 "$script_dir/dkms.conf" "$target/dkms.conf"
install -m 0644 "$script_dir/README.md" "$target/README.md"

dkms add -m "$package_name" -v "$package_version"
dkms install -m "$package_name" -v "$package_version" -k "$kernel_release"

install -m 0644 \
	"$script_dir/packaging/yoga-s740-vpc-poll.modules-load.conf" \
	"$autoload_target"

modprobe "$module_name"

echo "Installed $package_name/$package_version for $kernel_release."
echo "The module is active now and will load automatically at boot."
