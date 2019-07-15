#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
usage="$(basename "$0") [-h] [-updtbcvDTW] -- router script
where:
  -ml|--mac_lan         set qemu lan mac -mandatory
  -mw|--mac_wan         set qemu wan mac - mandatory
  -tl|--tap_lan_id      set tap lan id - mandatory
  -tw|--tap_wan_id      set tap wan id - mandatory
  -br|--bridge_id       set bridge id - mandatory
  -u|--user             set tap owner - mandatory
  -gw|--wan_gw          set wan tap ip - mandatory
  -i|--ifnet            set internet interface - mandatory
  -qb|--qemu_path       set qemu bin path - mandatory
  -qi|--qemu_image      set qemu image
  "
mac_lan=""
mac_wan=""
tap_lan_id=""
tap_wan_id=""
bridge_id=""
user=""
tap_wan_ip=""
if_net=""
qemu_bin=""
qemu_image=""
oui_file=""

if [ $# -eq 0 ]
then
  printf "$usage\n"
  exit 1
fi

valid_mac(){
  [[ $1 =~ ^([a-fA-F0-9]{2}:){5}[a-fA-F0-9]{2}$ ]] && echo "1" || echo "0"
}
valid_ip(){
  [[ $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] && echo "1" || echo "0"
}
valid_integer(){
  [[ $1 =~ ^-?[0-9]+$ ]] && echo "1" || echo "0"
}
mac_randomizer(){
  range=255

  number=$RANDOM
  numbera=$RANDOM
  numberb=$RANDOM

  let "number %= $range"
  let "numbera %= $range"
  let "numberb %= $range"

  octets="$1"

  octeta=`echo "obase=16;$number" | bc`
  octetb=`echo "obase=16;$numbera" | bc`
  octetc=`echo "obase=16;$numberb" | bc`

  macadd="${octets}:${octeta}:${octetb}:${octetc}"

  echo $macadd | tr [A-Z] [a-z]
}


while [[ $# -ge 1 ]]
do
  key="$1"
  case $key in
    -ml|--mac_lan) # qemu lan mac
      mac_lan="$2"
      shift
      ;;
    -mw|--mac_wan) # qemu wan mac
      mac_wan="$2"
      shift
      ;;
    -tl|--tap_lan_id) # lan tap id
      tap_lan_id="$2"
      shift
      ;;
    -tw|--tap_wan_id) # wan tap id
      tap_wan_id="$2"
      shift
      ;;
    -br|--bridge_id) # bridge id
      bridge_id="$2"
      shift
      ;;
    -u|--user) # tap owner
      user="$2"
      shift
      ;;
    -gw|--wan_gw) # wan tap ip
      tap_wan_ip="$2"
      shift
      ;;
    -i|--ifnet) # internet interface
      if_net="$2"
      shift
      ;;
    -qb|--qemu_path) # qemu bin path
      qemu_bin="$2"
      shift
      ;;
    -qi|--qemu-image) #qemu image
      qemu_image="$2"
      shift
      ;;
    --oui) # path to oui file
      oui_file="$2"
      shift
      ;;
    -h|--help)
      echo "$usage"
      exit 0
      ;;
    *)
      ;;
  esac
  shift
done
if [ "" != "${oui_file}" ] &&
  [ -f ${oui_file} ]
then
  target_oui=`shuf -n 1 ${oui_file}`
  if [ "" == "${mac_lan}" ]
  then
    mac_lan=`mac_randomizer ${target_oui}`
  fi
  if [ "" == "${mac_wan}" ]
  then
    mac_wan=`mac_randomizer ${target_oui}`
  fi
fi
if [ 0 -eq `valid_mac ${mac_lan}` ]
then
  printf "${RED}Invalid lan mac provided${NC} : ${mac_lan}\n"
  exit 1
fi
if [ 0 -eq `valid_mac ${mac_wan}` ]
then
  printf "${RED}Invalid wan mac provided${NC} : ${mac_wan}\n"
  exit 1
fi
if [ ! -f ${qemu_bin} ] || [ "" == "${qemu_bin}" ]
then
  printf "${RED}QEMU bin not found${NC}\n"
  exit 1
fi
if [ ! -f ${qemu_image} ] || [ "" == "${qemu_image}" ]
then
  printf "${RED}QEMU image not found${NC}\n"
  exit 1
fi

if [ "" == "${user}" ]
then
  printf "${RED}Invalid user provided${NC}\n"
  exit 1
fi
id -u ${user} &> /dev/null
if [ 0 -ne $? ]
then
  printf "${RED}Non-existant user provided${NC}\n"
  exit 1
fi
ifconfig ${if_net} &> /dev/null
ifret=$?
if [ "" == "$if_net" ] ||
  [ 0 -ne $ifret ]
then
  printf "${RED}Invalid internet interface${NC}\n"
  exit 1
fi
if [ 0 -eq `valid_ip "$tap_wan_ip"` ]
then
  printf "${RED}Invalid wan ip provided${NC}\n"
  exit 1
fi
if [ 0 -eq `valid_integer ${tap_lan_id}` ]
then
  printf "${RED}Invalid lan tap id provided${NC}\n"
  exit 1
fi
if [ 0 -eq `valid_integer ${tap_wan_id}` ]
then
  printf "${RED}Invalid wan tap id provided${NC}\n"
  exit 1
fi
if [ 0 -eq `valid_integer ${bridge_id}` ]
then
  printf "${RED}Invalid bridge id provided${NC}\n"
  exit 1
fi

tap_lan_interface="tap${tap_lan_id}"
tap_wan_interface="tap${tap_wan_id}"
bridge_interface="br${bridge_id}"

ifconfig ${tap_lan_interface} &> /dev/null
if [ 0 -eq $? ]
then
  printf "${RED}Lan tap interface already used : ${NC} ${tap_lan_interface}\n"
  exit 1
fi
ifconfig ${tap_wan_interface} &> /dev/null
if  [ 0 -eq $? ]
then
  printf "${RED}Wan tap interface already used : ${NC} ${tap_wan_interface}\n"
  exit 1
fi
ifconfig ${bridge_interface} &> /dev/null
if  [ 0 -eq $? ]
then
  printf "${RED}Bridge interface already used : ${NC} ${bridge_interface}\n"
  exit 1
fi

sudo tunctl -u "${user}" -t "${tap_wan_interface}"
sudo ifconfig "${tap_wan_interface}" "${tap_wan_ip}" up

sudo iptables -t nat -A POSTROUTING -o "${if_net}" -j MASQUERADE
sudo iptables -I FORWARD 1 -i "${tap_wan_interface}" -j ACCEPT
sudo iptables -I FORWARD 1 -o "${tap_wan_interface}" -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo sysctl -w net.ipv4.ip_forward=1

sudo brctl addbr "${bridge_interface}"
sudo ifconfig "${bridge_interface}" up
for i in `ls /proc/sys/net/bridge/bridge-nf*` ; do sudo sh -c "echo 0 > $i" ; done

sudo tunctl -u "${user}" -t "${tap_lan_interface}"
sudo ifconfig "${tap_lan_interface}" 0.0.0.0 up
sudo brctl addif "${bridge_interface}" "${tap_lan_interface}"

${qemu_bin} -M malta -no-reboot -kernel ${qemu_image} -nographic -m 256 \
  -net nic,vlan=${tap_lan_id},macaddr=${mac_lan} \
  -net tap,vlan=${tap_lan_id},ifname=${tap_lan_interface},script=no \
  -net nic,macaddr=${mac_wan} \
  -net tap,ifname=${tap_wan_interface},script=no

sudo brctl delif ${bridge_interface} ${tap_lan_interface}
sudo ifconfig ${tap_lan_interface} 0.0.0.0 down

sudo tunctl -d ${tap_lan_interface}
sudo tunctl -d ${tap_wan_interface}

sudo ifconfig "${bridge_interface}" down
sudo brctl delbr "${bridge_interface}"
