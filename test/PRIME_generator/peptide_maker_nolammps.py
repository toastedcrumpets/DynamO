#!/usr/bin/python2.7

from lxml import etree as ET
import sys
import os
import time
import random
import numpy as np
import subprocess
import prototype_positions as pp

#Turn on for full output from DynamO, and a .dcd file output
debug = False

def gauss():
    return str( random.gauss(0.0, 1.0) )

#############################################
###  Read in the command line parameters  ###
#############################################

nonglycine_SC    = list('ACDEFHIKLMNPQRSTVWY')
nonglycine_sites = list(nonglycine_SC) + ['NH', 'CH', 'CO']
sites            = list(nonglycine_sites) + ['G']
nonglycine_names = ['Alanine', 'Cysteine', 'Aspartic Acid', 'Glutamic Acid', 'Phenylalanine', 'Histidine', 'Isoleucine', 'Lysine', 'Leucine', 'Methionine', 'Asparagine', 'Proline', 'Glutamine', 'Arginine', 'Serine', 'Threonine', 'Valine', 'Tryptophan', 'Tyrosine' ] + ['Nitrogen+Hydrogen', 'Carbon+Hydrogen', 'Carbon+Oxygen']

try:
    filename = str(sys.argv[3])
except:
    filename = 'PRIME_peptide'

psf_fn = filename + '.psf'
xml_fn = filename + '.xml'

try:
    temperature = sys.argv[2]
except:
    temperature = '1.0'

try:
    sys.argv[1] = [ letter.upper() for letter in sys.argv[1] ]
    if ( sys.argv[1] == list('N16N') ):
        sequence = list('AYHKKCGRYSYCWIPYDIERDRYDNGDKKC')
    elif ( sys.argv[1][:5] == list('ALPHA') ):
        sequence = list('ACDEFHIKLMNPQRSTVWY')
    else:
        sequence = list(sys.argv[1])
        assert [ sites.index(residue) for residue in sequence]
except (ValueError, IndexError):
    sys.exit('Run as ./peptide_maker.py (sequence) [temperature kT = 1.0] [xml_fn = PRIME_peptide].')

date               = time.strftime('%X %x %Z')
box_size_per_res   = 10.0
box_pad            = 5.0
box_size           = 2*box_pad + len(sequence)*box_size_per_res
n_residues         = len(sequence)
n_bb_sites         = 3*n_residues
n_sc_sites         = n_residues - sequence.count('G')
n_sites            = n_bb_sites + n_sc_sites
dcd_temp_dir       = "tempDCDs"

expanded_sequence = []
sidechain_IDs = []
for letter in sequence:
    expanded_sequence += ['NH','CH','CO']
    if letter != 'G':
        sidechain_IDs.append(len(expanded_sequence))
        expanded_sequence.append(letter)

nonglycine_expanded_sequence = filter(lambda a: a != 'G', expanded_sequence) #'Real' list, e.g. AGA gives NH,CH,CO,A,NH,CH,CO,NH,CH,CO,A

print 'Sequence:' , ''.join(sequence)
print 'File name:' , xml_fn , '\n'

##########################
###      Geometry      ###
##########################

coords = np.zeros([ len(nonglycine_expanded_sequence), 3 ], dtype=float)
j=0
for i_res, res in enumerate(sequence):

    for i_atom in range(3):
        bb_only_index = i_res*3 + i_atom
        coords[j] = pp.BB_33[bb_only_index]
        j += 1

    if res != 'G':
        res_number = nonglycine_sites.index(res)

        coords[j] = pp.SC_33[ res_number ][ i_res ]
        j += 1

#############################################
###               Set up XML              ###
#############################################

DynamOconfig = ET.Element    ( 'DynamOconfig', attrib = {'version' : '1.5.0'} )

Simulation   = ET.SubElement ( DynamOconfig, 'Simulation', attrib = {'lastMFT':"-nan"} )
Properties   = ET.SubElement ( DynamOconfig, 'Properties' )
ParticleData = ET.SubElement ( DynamOconfig, 'ParticleData' )

####ParticleData section####
for ID in range( n_sites ):
    ET.SubElement( ParticleData, 'Pt', attrib = {'ID' : str(ID) } )

    ET.SubElement( ParticleData[ID], 'P', attrib = {"x":str( coords[ID][0] ), "y":str( coords[ID][1] ), "z":str( coords[ID][2] )} )
    ET.SubElement( ParticleData[ID], 'V', attrib = {"x":gauss(), "y":gauss(), "z":gauss()} )

####Simulation section####

Scheduler      = ET.SubElement ( Simulation, 'Scheduler',      attrib = {'Type':'NeighbourList'} )
SimulationSize = ET.SubElement ( Simulation, 'SimulationSize', attrib = dict(zip(['x','y','z'], [str(box_size)]*3)) )
BC             = ET.SubElement ( Simulation, 'BC',             attrib = {'Type':'PBC'} )
Genus          = ET.SubElement ( Simulation, 'Genus')
Topology       = ET.SubElement ( Simulation, 'Topology' )
Interactions   = ET.SubElement ( Simulation, 'Interactions')
Locals         = ET.SubElement ( Simulation, 'Locals' )
Globals        = ET.SubElement ( Simulation, 'Globals' )
SystemEvents   = ET.SubElement ( Simulation, 'SystemEvents' )
Dynamics       = ET.SubElement ( Simulation, 'Dynamics', attrib = {'Type':'Newtonian'} )

Sorter = ET.SubElement ( Scheduler, 'Sorter', attrib = {'Type':'BoundedPQMinMax3'} )

#Add each species and its list of IDs to the XML tree
PRIME_species = ET.SubElement( Genus, 'Species', attrib = {'Mass':'PRIMEData', 'Name':'PRIMEGroups', 'Type':'Point'} )
ET.SubElement( PRIME_species, 'IDRange', attrib = {'Type':'All'} )

temp = ET.SubElement( Globals, 'Global', attrib = {'Type':'Cells','Name':'SchedulerNBList','NeighbourhoodRange':'7.400000000000e+00'})
ET.SubElement( temp, 'IDRange', attrib = {'Type':'All'})

#Interactions section
PRIME_BB=ET.SubElement( Interactions, 'Interaction', attrib = {'Type':'PRIME_BB', 'Name':'Backbone', 'Topology':"PRIMEData"} )
ET.SubElement( PRIME_BB, 'IDPairRange', attrib = {'Type':'All'} )

#Topology section
Structure = ET.SubElement ( Topology, 'Structure', attrib = {'Type':'PRIME', 'Name':'PRIMEData'} )
Molecule  = ET.SubElement ( Structure, 'Molecule', attrib = {'StartID':'0', 'Sequence':''.join(sequence)} )

######################
#  Create PSF files  #
######################

print "----------------------------------------------------"
print "WARNING PSF FILE-GENERATOR IS OUT OF DATE AND WRONG."
print "----------------------------------------------------"

psf_atoms_section = ""
psf_bonds_section = ""

#Backbone atoms
for i_res, res in enumerate(sequence):
    for i_local_atom, atom in enumerate(['NH', 'CH', 'CO']):
        i_atom = i_res*3 + i_local_atom
        psf_atoms_section += "{0: >8d} {1: <4} {2: <4d} {3: <4} {4: <4} {4: <4} {5: >10} {6: >13} {7: >11}\n".format(i_atom+1, str(0), i_res, res, atom, "0.000000", "0.0000", "0")

#SC atoms
for i_res, res in enumerate(nonglycine_expanded_sequence[n_bb_sites:]):
    i_atom = n_bb_sites + i_res
    psf_atoms_section += "{0: >8d} {1: <4} {2: <4d} {3: <4} {4: <4} {4: <4} {5: >10} {6: >13} {7: >11}\n".format(i_atom+1, str(0), i_res, res, res, "0.000000", "0.0000", "0")

#BB bonds
for i_bb_site in range(1,n_bb_sites):
    psf_bonds_section += "{0: >8d}{1: >8d}".format(i_bb_site, i_bb_site+1)

    if len(psf_bonds_section) - psf_bonds_section.rfind("\n") > 63:
        psf_bonds_section += "\n"

#SC bonds
for i_res, res in enumerate(sequence):
    if res != 'G':
        i_bb_site = i_res*3 + 2
        i_sc_site = n_bb_sites + 1 + i_res - sequence[:i_res].count('G')
        psf_bonds_section += "{0: >8d}{1: >8d}".format(i_bb_site, i_sc_site)

        if len(psf_bonds_section) - psf_bonds_section.rfind("\n") > 63:
            psf_bonds_section += "\n"

with open(psf_fn, 'w') as psf_file:
    psf_file.write("PSF\n\n\t1 !NTITLE\n REMARKS " + ''.join(sequence) + " STRUCTURE FILE\n REMARKS DATE: " + date + "\n\n")
    psf_file.write("{0: >8d}".format(n_bb_sites+n_sc_sites) + " !NATOM\n" + psf_atoms_section + "\n")
    psf_file.write("{0: >8d}".format(n_bb_sites-1+n_sc_sites) + " !NBOND\n" + psf_bonds_section + "\n\n")

####################
#  Write XML file  #
####################

input_file = open(xml_fn, 'w')
input_file.write('<!-- DynamO input file contains the PRIME20 model of the sequence: ' + ''.join(sequence) + '. -->\n')
input_file.write('<!-- Created on ' +date + '. -->\n')
[ input_file.write(ET.tostring(DynamOconfig, pretty_print=True)) ]
input_file.close()

#Add thermostat and rescale via dynamod:
thermostat_command = [ 'dynamod',  '-T', temperature, '-r', temperature, '-o', xml_fn, '-Z', xml_fn ]
print "Running this command:", " ".join(thermostat_command)
if debug:
    print subprocess.check_output(thermostat_command)
else:
    silent_stdout = subprocess.check_output(thermostat_command)

#Check config is valid with dynamod:
run_command = ['dynamod', xml_fn, "--check"]
print "Running this command:", " ".join(run_command)
if debug:
    print subprocess.check_output(run_command)
else:
    silent_stdout = subprocess.check_output(run_command)

if debug:
    #Create trajectory file
    traj_command = ['dynamo2xyz', xml_fn]
    print "Running this command:", " ".join(traj_command)
    with open('traj.xyz', 'w') as trajfile:
        xyz = subprocess.check_output(traj_command)
        trajfile.write(xyz)

    convert_command = ["catdcd", "-o", dcd_temp_dir+"/dynamO_traj.dcd", "-xyz", "traj.xyz"]
    subprocess.check_output(convert_command)
    print "Running this command:", " ".join(convert_command)

    os.remove("traj.xyz")
