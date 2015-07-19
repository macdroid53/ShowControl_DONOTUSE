#!/usr/bin/env python3
__author__ = 'mac'

import os, sys, inspect
import types
import argparse

from PyQt5 import Qt, QtCore, QtGui, QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtWidgets import *

import xml.etree.ElementTree as ET
from Cues import CueList

cues = CueList('/home/mac/Shows/Scrooge/Scrooge Moves.xml')

print(cues.cuelist._root)
root = cues.cuelist.getroot()
print(root)

#for child in root:
#    print(child.tag, child.attrib)

cueidx = 2

cuenum = '{0:03}'.format(cueidx)


#cuetomod = cues.cuelist.find("cue[@num='001']")
cuetomod = cues.cuelist.find("cue[@num='"+cuenum+"']")
print(cuetomod.keys())

print(cuetomod.find("Move").text)
print(cuetomod.find("Id").text)

newrow = ['6', '1', ' 1', '1', '1A', 'Pauline enters', 'Lights up']

cuetomod.find("Move").text = newrow[0]

print(cuetomod.find("Move").text)
