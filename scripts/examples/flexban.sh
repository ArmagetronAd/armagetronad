#!/bin/bash

# This script can ban and kick players based on more flexible
# criteria than the game can. It reads ban rules from the configuration
# script flexban_cfg.sh, which really is just a shell script in which you can
# execute the following commands. All commands operate on the player that last
# entered the server. You can use all the usual bash flow control, for example
# grep_geoip Australia && ban 60 "No Aussies!"
# bans all australians, while
# grep_geoip Australia || ban 60 "Aussies only!"
# bans all but Australians, and
# grep_geoip Australia || grep_whois Bruce && ban 60 "Aussie Bruces only!"
# bans everyone not from australia with Bruce appearing in their whois entry.
# (Sorry for the silly examples.)

# Whois lookup uses the whois console command, available under that name in most
# distributions. The lookup uses the network and takes about half a second.

# Basic geoip lookup uses 'geoiplookup' according to this article:
# http://blog.rayfoo.info/2010/07/doing-geolocation-lookups-in-command-line
# you can set GEOIP_ARG to make it use a non-default database (default only
# lists countries). The lookup uses a local database that needs updating from time
# to time.

# greps geoip for pattern, returns true on match. All arguments are passed to grep.
function grep_geoip()
{
    test ${BANME} = true && return 1

    set_geoip
    echo ${GEOIP} | grep "$*" 2>&1 > /dev/null
}

# greps whois for pattern, returns true on match. All arguments are passed to grep.
function grep_whois()
{
    test ${BANME} = true && return 1

    set_whois
    echo ${WHOIS} | grep "$*" 2>&1 > /dev/null
}

# checks for IP ranges, returns true if the IP is in the range.
# Range syntax: base_ip/significant bits, i.e.
# 192.168.0.0/16
# matches all IPs of the form 192.168.x.x.
function check_range()
{
    if ! echo $1 | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+\/[0-9]\+$" > /dev/null 2>&1; then
        echo "Not an IP range: $1" 1>&2
        return 1
    fi

    if ! echo ${IP} | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+$" > /dev/null 2>&1; then
        echo "SAY Internal flexban BUG: Not a valid IP: ${IP}" 1>&2
        return 1
    fi

    check_range_core $(echo $IP | sed -e 's,\., ,g') $(echo $1 | sed -e 's,\., ,g' -e 's,/, ,')
}

# bans the player currently in investigation. Usage:
# ban <time> "<reason>"
function ban()
{
    test ${BANME} = true && return

    TIME=$1
    REASON=$2
    test -z "${TIME}" && TIME=60
    test -z "${REASON}" && REASON="Flexban rule match"
    echo BAN_IP ${IP} ${TIME} ${REASON}
    LASTBAN=${IP}

    BANME=true
}

# whitelists the player currently in investigation; further ban commands are ignored.
function whitelist()
{
    # just set the flag that cancels all checks
    BANME=true
}

#############################################
# 
# End of commands to use in the configuration
# script, the rest are for internal use only.
#
#############################################

# IP range check core, syntax
# check_range_core CHECK_IP1 CHECK_IP2 CHECK_IP3 CHECK_IP4 RANGE_IP1 RANGE_IP2 RANGE_IP3 RANGE_IP4 BITS
function check_range_core()
{
    IP_NUM=$(( $4 + 256*($3 + 256* ($2 + 256 * $1 ) ) ))
    CHECK_NUM=$(( $8 + 256*($7 + 256* ($6 + 256 * $5 ) ) ))
    RANGE=$(( (1 << $9) - 1 ))
    CHECK_NUM=$(( ${CHECK_NUM} & (~${RANGE}) ))
    UPPER_END=$(( ${CHECK_NUM} | ${RANGE} ))
    # echo ${CHECK_NUM} ${UPPER_END} ${RANGE} ${IP_NUM}
    test ${IP_NUM} -ge ${CHECK_NUM} && test ${IP_NUM} -lt ${UPPER_END}
}

# sets GEOIP to the output of geoiplookup
function set_geoip()
{
    test -z "${GEOIP}" && GEOIP=$(geoiplookup ${GEOIP_ARG} ${IP})
}

# sets WHOIS to the output of whois
function set_whois()
{
    test -z "${WHOIS}" && WHOIS=$(whois ${IP})
}

# processes the player_entered messages individually
function process_core()
{
    IP=$3
    #echo ${IP}
    PLAYER=$2

    WHOIS=""
    GEOIP=""
    GEOIP_ARG=""
    BANME=false

    # load configuration, execute commands
    . flexban_cfg.sh
}

# processes the player_entered messages
function process()
{
    while LINE=$(line); do
        process_core ${LINE}
    done
}

# for debugging, set the path like it would be when run from the
# game itself
# export PATH="${PATH}:$(dirname $(dirname $(dirname $0)))/config/"
# set -x

LASTBAN=""

grep --line-buffered "^PLAYER_ENTERED" | process


