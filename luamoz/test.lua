require('luamoz')

secret_word="secret"
ip="192.168.1.1"

pass=moz_calc_pass(ip,secret_word,24)

print(pass)

print(moz_validate_pass(pass,ip,secret_word))