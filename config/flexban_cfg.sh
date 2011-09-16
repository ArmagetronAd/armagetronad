# flexban configuration

# ban hotspot shield users
. flexban_hotspotshield_cfg.sh

grep_whois Barrios && ban 60 "test ban"
check_range 190.253.0.0/16 && ban 60 "range ban 1"
#check_range 190.253.0.0 && ban 60 "range ban 2"

#ban

