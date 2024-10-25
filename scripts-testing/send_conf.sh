#!/usr/bin/bash
# Orig
# mosquitto_pub -t '/power/conf' -m '{
#     "q(0,0)": 0.002,
#     "q(0,1)": 0,
#     "q(1,0)": 0,
#     "q(1,1)": 0.01,
#     "r(0,0)": 1e4,
#     "r(0,1)": 0,
#     "r(1,0)": 0,
#     "r(1,1)": 0.1
# }'

# Works ok, but a bit slow to settle.
# mosquitto_pub -t '/power/conf' -m '{
#     "q(0,0)": 0.002,
#     "q(0,1)": 0,
#     "q(1,0)": 0,
#     "q(1,1)": 0.1,
#     "r(0,0)": 500,
#     "r(0,1)": 0,
#     "r(1,0)": 0,
#     "r(1,1)": 0.01
# }'

mosquitto_pub -t '/power/conf' -m '{
    "q(0,0)": 0.002,
    "q(0,1)": 0,
    "q(1,0)": 0,
    "q(1,1)": 0.1,
    "r(0,0)": 100,
    "r(0,1)": 0,
    "r(1,0)": 0,
    "r(1,1)": 0.01
}'

# mosquitto_pub -t '/power/conf' -m '{
#     "q(0,0)": 1,
#     "q(0,1)": 0,
#     "q(1,0)": 0,
#     "q(1,1)": 1,
#     "r(0,0)": 1,
#     "r(0,1)": 0,
#     "r(1,0)": 0,
#     "r(1,1)": 1
# }'