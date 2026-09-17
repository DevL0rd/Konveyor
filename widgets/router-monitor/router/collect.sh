#!/bin/sh
# Linux-Router-Monitor :: remote collector
# Runs ON the router (Asuswrt-Merlin / BusyBox ash). Prints a flat, easy-to-parse
# text dump; the desktop side turns it into JSON, computes rates, merges AdGuard.
#
# Args: $1 ping target   $2 do_ping (1/0)   $3 do_slow (1/0)   $4 do_static (1/0)
#   FAST section (always): CPU, memory, conntrack, interface counters, and the
#     bulk per-client throughput dump (wl bs_data) -- changes second to second.
#   SLOW section (do_slow): WAN state, per-radio wl status/airtime/dfs/temp,
#     leases, arp, firewall, per-client signal/link-rate -- change over time.
#   STATIC section (do_static): model/fw/lan_ip, ssid/radio/txpower, wired ports
#     -- only change on reconfiguration, so the desktop fetches them only on a
#     reconnect (offline -> online), not on a timer.
#   PING: slowest of all (~2s), requested even less often.

PING_TARGET="${1:-8.8.8.8}"
DO_PING="${2:-1}"
DO_SLOW="${3:-1}"
DO_STATIC="${4:-0}"

WLIFACES="eth7 eth8 eth9 eth10"
band_of() {
	case "$1" in
		eth7) echo "5GHz-1";; eth8) echo "5GHz-2";; eth9) echo "6GHz";; eth10) echo "2.4GHz";; *) echo "$1";;
	esac
}
wlidx_of() {
	case "$1" in
		eth7) echo 0;; eth8) echo 1;; eth9) echo 2;; eth10) echo 3;; *) echo "";;
	esac
}

echo "TS=$(date +%s)"

############ FAST: system ############
read L1 L5 L15 PROCS REST < /proc/loadavg
echo "load1=$L1"; echo "load5=$L5"; echo "load15=$L15"; echo "procs=$PROCS"
echo "uptime=$(awk '{print $1}' /proc/uptime)"
echo "cores=$(grep -c '^processor' /proc/cpuinfo)"
grep '^cpu' /proc/stat | while read -r line; do echo "STAT $line"; done
awk '/^MemTotal:/{print "mem_total="$2}
     /^MemFree:/{print "mem_free="$2}
     /^MemAvailable:/{print "mem_avail="$2}
     /^Buffers:/{print "mem_buffers="$2}
     /^Cached:/{print "mem_cached="$2}
     /^SwapTotal:/{print "swap_total="$2}
     /^SwapFree:/{print "swap_free="$2}' /proc/meminfo
[ -f /sys/class/thermal/thermal_zone0/temp ] && echo "cpu_temp_milli=$(cat /sys/class/thermal/thermal_zone0/temp)"
echo "conntrack=$(cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null)"
echo "conntrack_max=$(cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null)"

############ FAST: interface counters (for throughput rates) ############
WANIF=$(nvram get wan0_pppoe_ifname); [ -z "$WANIF" ] && WANIF=$(nvram get wan0_ifname)
echo "wan_ifname=$WANIF"
awk -F: 'NR>2{name=$1; gsub(/[ \t]/,"",name); split($2,a," ");
            if(name!="" && a[1]!="") print "IF "name" "a[1]" "a[9]}' /proc/net/dev

############ FAST: per-client live throughput ############
# `wl bs_data` is a BULK dump: one call per radio lists every client with its
# live Data Mbps + PHY rate. Scales with radios (4), not client count.
for ifc in $WLIFACES; do
	band=$(band_of "$ifc")
	wl -i "$ifc" bs_data 2>/dev/null | awk -v band="$band" -v ifc="$ifc" '
		$1 ~ /^[0-9A-Fa-f:][0-9A-Fa-f:]/ && NF>=3 {
			n++; print "STA mac=" $1 " band=" band " phy=" $2 " data_mbps=" $3
		}
		END { print "RCOUNT ifc=" ifc " band=" band " clients=" n+0 }
	'
done

############ SLOW: WAN state + per-radio dynamic meta + leases/firewall ############
if [ "$DO_SLOW" = "1" ]; then
	echo "wan_proto=$(nvram get wan0_proto)"
	echo "wan_ip=$(nvram get wan0_ipaddr)"
	echo "wan_gw=$(nvram get wan0_gateway)"
	echo "wan_dns=$(nvram get wan0_dns)"
	echo "wan_state=$(nvram get wan0_state_t)"

	for ifc in $WLIFACES; do
		band=$(band_of "$ifc")
		# one awk over `wl status` -> chan/width/noise (was 3 sed pipelines)
		ststat=$(wl -i "$ifc" status 2>/dev/null | awk '
			/Primary channel:/ { s=$0; sub(/.*Primary channel: /,"",s); sub(/[^0-9].*/,"",s); chan=s }
			/Chanspec:/ { for(i=1;i<=NF;i++) if($i ~ /^[0-9]+MHz$/) width=$i }
			/noise:/    { for(i=1;i<=NF;i++) if($i=="noise:") noise=$(i+1) }
			END { printf "chan=%s width=%s noise=%s", chan, width, noise }')
		# one awk over `wl chanim_stats` -> busy/glitch/badplcp (was 1 + 3 awks)
		cimstat=$(wl -i "$ifc" chanim_stats 2>/dev/null | awk '
			NF>=15 && $1!="chanspec" { b=$15; g=$11; p=$12 }
			END { printf "busy=%s glitch=%s badplcp=%s", b, g, p }')
		temp=$(wl -i "$ifc" phy_tempsense 2>/dev/null | awk '{print $1}')
		dfs=$(wl -i "$ifc" dfs_status 2>/dev/null | sed -n 's/state \([A-Za-z()-]*\).*/\1/p' | head -1)
		echo "RADIO ifc=$ifc band=$band temp=$temp $ststat $cimstat dfs=$dfs"
	done

	# per-client signal + link rates (change slowly -> only on the slow tick)
	for ifc in $WLIFACES; do
		for mac in $(wl -i "$ifc" assoclist 2>/dev/null | awk '{print $2}'); do
			wl -i "$ifc" sta_info "$mac" 2>/dev/null | awk -v mac="$mac" '
				/smoothed rssi:/       { rssi = $NF }
				/rate of last tx pkt:/ { txr = $6 }
				/rate of last rx pkt:/ { rxr = $6 }
				END { print "STASLOW mac=" mac " rssi=" rssi " txrate=" txr " rxrate=" rxr }
			'
		done
	done

	if [ -f /var/lib/misc/dnsmasq.leases ]; then
		while read -r exp mac ip name rest; do
			echo "LEASE mac=$mac ip=$ip name=$name"
		done < /var/lib/misc/dnsmasq.leases
	fi
	awk 'NR>1 && $4!="00:00:00:00:00:00"{print "ARP "$4" "$3}' /proc/net/arp
	iptables -S FORWARD 2>/dev/null | grep -io 'mac-source [0-9A-Fa-f:]\{17\}' | awk '{print "BLOCKED "$2}'
	echo "wrs_protect=$(nvram get wrs_protect_enable)"
	echo "wrs_mals=$(nvram get wrs_mals_enable)"
	echo "SLOW=1"
fi

############ STATIC: identity + ssid/radio/txpower + ports (reconnect only) ############
if [ "$DO_STATIC" = "1" ]; then
	echo "model=$(nvram get productid)"
	echo "fw=$(nvram get buildno).$(nvram get extendno)"
	echo "lan_ip=$(nvram get lan_ipaddr)"
	for p in eth0 eth1 eth2 eth3 eth4 eth5 eth6; do
		media=$(ethctl "$p" media-type 2>/dev/null | grep -i "Link is" | head -1)
		[ -n "$media" ] && echo "PORT $p ${media##*Link is }"
	done
	for ifc in $WLIFACES; do
		band=$(band_of "$ifc"); idx=$(wlidx_of "$ifc")
		echo "RSTATIC band=$band ssid=$(nvram get wl${idx}_ssid) radio=$(nvram get wl${idx}_radio) txpower=$(nvram get wl${idx}_txpower)"
	done
	echo "STATIC=1"
fi

############ PING (least often) ############
if [ "$DO_PING" = "1" ]; then
	png=$(ping -c 3 -w 4 "$PING_TARGET" 2>/dev/null)
	echo "ping_loss=$(echo "$png" | sed -n 's/.*, \([0-9]*\)% packet loss.*/\1/p')"
	echo "ping_rtt=$(echo "$png" | sed -n 's#.*= [0-9.]*/\([0-9.]*\)/.*#\1#p')"
fi

echo "END=1"
