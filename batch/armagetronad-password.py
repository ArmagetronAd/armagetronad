#!/usr/bin/env python

# this program makes password files for the web server

import md5

print "Armagetron Advanced Password Generator"
print

def get_input(prompt, defaultvalue):
    inputt = raw_input(prompt + " " + defaultvalue + ": ")
    if len(inputt) < 1:
        inputt = defaultvalue

    return inputt

user = get_input("User", "guest")
domain = get_input("Domain", "mydomain.com")
password = get_input("Password", "guest")
passwordfile = get_input("Password file", "/etc/armagetronad-dedicated/web_password.cfg")

passwordline = user + ":" + domain + ":" + password
passwordhash = md5.md5(passwordline).hexdigest()

passwordlinehashed = user + ":" + domain + ":" + passwordhash

print passwordlinehashed

fp = open(passwordfile, "a")
fp.write(passwordlinehashed + "\n")
fp.close()

print "Done.  Don't forget to change server_name in settings_web.cfg to match the domain you just gave me!"
