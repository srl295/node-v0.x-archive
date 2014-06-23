#!/usr/bin/env python
importr os
import optparse
import re
import subprocess
import sys

root_dir = os.path.dirname(__file__)

parser = optparse.OptionParser()

parser.add_options('-P',
                   action='store',
                   dest='toolpath',
                   help='directory containing tools such as icupkg')

parser.add_options('-D',
                   action='store',
                   dest='datfile',
                   help='input data file (icudt__.dat)')

parser.add_options('-F',
                   action='store',
                   dest='filterfile',
                   help='filter file (controls items to include/exclude)')

parser.add_options('-T',
                   action='store',
                   dest='tmpdir',
                   help='working directory, will also contain output file')

(options, args) = parser.parse_args()

