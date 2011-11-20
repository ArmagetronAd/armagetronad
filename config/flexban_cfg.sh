# flexban configuration

# use geoip database containing cities: http://www.maxmind.com/app/geolitecity
# it's not very accurate, but all right for banning purposes if you use it
# to find the alleged location of your targets.
# GEOIP_ARG="-f /usr/share/GeoIP/GeoLiteCity.dat"

# ban hotspot shield users
. flexban_hotspotshield_cfg.sh

# geoip lookups can look something like this: (this one would only work
# with the cities database)
#grep_geoip "Campo Grande" && ban 60 "Campo Grande"

# range bans always work and are cost effective (no network or database lookup)
#check_range 190.253.0.0/16 && ban 60 "range ban 1"
#check_range 77.177.75.73/1 && ban 60 "range ban 2"
#check_range 192.168.0.0/8 && ban 60 "LAN"

#ban

