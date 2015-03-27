#!/usr/bin/env python
#   dynamo:- Event driven molecular dynamics simulator 
#   http://www.dynamomd.org
#   Copyright (C) 2009  Marcus N Campbell Bannerman <m.bannerman@gmail.com>
#
#   This program is free software: you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 3 as published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
import os
import math
import sys
import argparse
import xml.etree.ElementTree as ET

swaps=1000
swap_time=1

#This may not exactly correspond to the real finish time, it may be +-
#swaptime
finish_time=float(swaps)*swap_time 

def isclose(a,b,tol):
    return (abs(a-b) <= abs(tol*a)) or (abs(a-b) <= abs(tol*b))

parser = argparse.ArgumentParser(description="Test harness for running DynamO tests")
parser.add_argument('--dynarun', dest="dynarun_cmd", help='The location of the built dynarun command to be tested', required=True)
parser.add_argument('--dynamod', dest="dynamod_cmd", help='The location of the built dynamod command to be tested', required=True)
parser.add_argument('--dynahist_rw', dest="dynahist_rw_cmd", help='The location of the built dynahist_rw command to be tested', required=True)
args = parser.parse_args()

for exe in [args.dynahist_rw_cmd, args.dynamod_cmd, args.dynarun_cmd]:
    if not(os.path.isfile(exe) and os.access(exe, os.X_OK)):
        raise RuntimeError("Failed to find "+exe)
        
import subprocess

###### INITIALISATION
Temperatures=[1.0, 2.0, 5.0, 10.0]
for i,T in enumerate(Temperatures):
    cmd=[args.dynamod_cmd, "-m2", "-T"+str(T), "-oc"+str(i)+".xml"]
    print " ".join(cmd)
    subprocess.call(cmd)

###### EQUILIBRATION
cmd=[args.dynarun_cmd, "--engine=2", "-oc%ID.xml", "--out-data-file=o%ID.xml", "-N4", "-i"+str(swap_time), "-f"+str(finish_time)]+["c"+str(i)+".xml" for i in range(len(Temperatures))]
print " ".join(cmd)
subprocess.call(cmd)


###### PRODUCTION
cmd=[args.dynarun_cmd, "--engine=2", "-oc%ID.xml", "--out-data-file=o%ID.xml", "-N4", "-i"+str(swap_time), "-LIntEnergyHist", "-f"+str(finish_time)]+["c"+str(i)+".xml" for i in range(len(Temperatures))]
print " ".join(cmd)
subprocess.call(cmd)

import re
replex_calls=None
with open("replex.stats") as f:
    for line in f:
        match = re.search("Number_of_replex_cycles ([0-9]+)", line)
        if match:
            replex_calls = int(match.group(1))
            break

if replex_calls is None:
    raise RuntimeError("Could not parse the number of replex calls performed")

if abs(replex_calls - swaps) > 1:
    raise RuntimeError("The number of actual swaps ("+str(replex_calls)+") is different to the number of requested swaps +("+str(swaps)+")")

for i,T in enumerate(Temperatures):
    outputfile="o"+str(i)+".xml"
    xmldoc=ET.parse(outputfile)

    measured_T=float(xmldoc.getroot().find(".//Temperature").attrib["Mean"])
    if not isclose(measured_T, T, 4e-2):
        raise RuntimeError("Simulation Temperature is different to what is expected:"+str(measured_T)+"!="+str(T))
    
    simtime=float(xmldoc.getroot().find(".//Duration").attrib["Time"])
    print math.sqrt(min(Temperatures)/T)
    expected_simtime = swap_time * replex_calls * math.sqrt(min(Temperatures)/T)
    if not isclose(simtime, expected_simtime, 1e-4):
        raise RuntimeError("Simulation duration is different to what is expected:"+str(simtime)+"!="+str(expected_simtime))
        