#!/usr/bin/python
#
# Copyright (C) 2014 IBM Corporation and Others. All Rights Reserved.
#
# @author Steven R. Loomis <srl@icu-project.org>
#
# This tool slims down an ICU data (.dat) file according to a config file.
#
# See: http://bugs.icu-project.org/trac/ticket/10922
#
# Usage:
#  Use "-h" to get help options.

import sys
import shutil
# for utf-8
reload(sys)
sys.setdefaultencoding("utf-8")

import argparse
import os
import json
import re

endian=sys.byteorder

parser = argparse.ArgumentParser(description="ICU Datafile repackager.  Example of use:    \"mkdir tmp ;  python icutrim.py -D ~/Downloads/icudt53l.dat -T tmp -F trim_en.json -O icudt53l.dat\"       you will then find a smaller icudt53l.dat in 'tmp'. ",
                                 epilog="ICU tool, http://icu-project.org - master copy at http://source.icu-project.org/repos/icu/tools/trunk/scripts/icutrim.py")

parser.add_argument("-P","--tool-path",
                    action="store",
                    dest="toolpath",
                    help="set the prefix directory for ICU tools")

parser.add_argument("-D","--input-file",
                    action="store",
                    dest="datfile",
                    help="input data file (icudt__.dat)",
                    required=True)

parser.add_argument("-F","--filter-file",
                    action="store",
                    dest="filterfile",
                    help="filter file (JSON format)",
                    required=True)

parser.add_argument("-T","--tmp-dir",
                    action="store",
                    dest="tmpdir",
                    help="working directory.",
                    required=True)

parser.add_argument("--delete-tmp",
                    action="count",
                    dest="deltmpdir",
                    help="delete working directory.",
                    default=0)

parser.add_argument("-O","--outfile",
                    action="store",
                    dest="outfile",
                    help="outfile  (NOT a full path)",
                    required=True)

parser.add_argument("-v","--verbose",
                    action="count",
                    default=0)

parser.add_argument('-e', '--endian', action='store', dest='endian', help='endian, big, little or host, your default is "%s".' % endian, default=endian, metavar='endianness')


args = parser.parse_args()

if args.verbose>0:
    print "Options: "+str(args)

if (os.path.isdir(args.tmpdir) and args.deltmpdir):
  if args.verbose>1:
    print "Deleting tmp dir %s.." % (args.tmpdir)
  shutil.rmtree(args.tmpdir)

if not (os.path.isdir(args.tmpdir)):
    os.mkdir(args.tmpdir)
else:
    print "Please delete tmpdir %s before beginning." % args.tmpdir
    sys.exit(1)

if args.endian not in ("big","little","host"):
    print "Unknown endianness: %s" % args.endian
    sys.exit(1)

if args.endian is "host":
    args.endian = endian

if not os.path.isdir(args.tmpdir):
    print "Error, tmpdir not a directory: %s" % (args.tmpdir)
    sys.exit(1)

if not os.path.isfile(args.filterfile):
    print "Filterfile doesn't exist: %s" % (args.filterfile)
    sys.exit(1)

if not os.path.isfile(args.datfile):
    print "Datfile doesn't exist: %s" % (args.datfile)
    sys.exit(1)

if not args.datfile.endswith(".dat"):
    print "Datfile doesn't end with .dat: %s" % (args.datfile)
    sys.exit(1)

outfile = os.path.join(args.tmpdir, args.outfile)

if os.path.isfile(outfile):
    print "Error, output file does exist: %s" % (outfile)
    sys.exit(1)

if not args.outfile.endswith(".dat"):
    print "Outfile doesn't end with .dat: %s" % (args.outfile)
    sys.exit(1)

dataname=args.outfile[0:-4]


## TODO: need to improve this. Quotes, etc.
def runcmd(tool, cmd, doContinue=False):
    if(args.toolpath):
        cmd = os.path.join(args.toolpath, tool) + " " + cmd
    else:
        cmd = tool + " " + cmd

    if(args.verbose>4):
        print "# " + cmd

    rc = os.system(cmd)
    if rc is not 0 and not doContinue:
        print "FAILED: %s" % cmd
        sys.exit(1)
    return rc

## STEP 0 - read in json config
fi= open(args.filterfile, "rb")
config=json.load(fi)
fi.close()

if (args.verbose > 6):
    print config

if(config.has_key("comment")):
    print "%s: %s" % (args.filterfile, config["comment"])

## STEP 1 - copy the data file, swapping endianness
endian_letter = args.endian[0]


runcmd("icupkg", "-t%s %s %s""" % (endian_letter, args.datfile, outfile))

## STEP 2 - get listing
listfile = os.path.join(args.tmpdir,"icudata.lst")
runcmd("icupkg", "-l %s > %s""" % (outfile, listfile))

fi = open(listfile, 'rb')
items = fi.readlines()
items = [items[i].strip() for i in range(len(items))]
fi.close()

itemset = set(items)

if (args.verbose>1):
    print "input file: %d items" % (len(items))

# list of all trees
trees = {}
RES_INDX = "res_index.res"
remove = None
# remove - always remove these
if config.has_key("remove"):
    remove = set(config["remove"])
else:
    remove = set()

# keep - always keep these
if config.has_key("keep"):
    keep = set(config["keep"])
else:
    keep = set()

def queueForRemoval(tree):
    global remove
    if not config.has_key("trees"):
        # no config
        return
    if not config["trees"].has_key(tree):
        return
    mytree = trees[tree]
    if(args.verbose>0):
        print "* %s: %d items" % (tree, len(mytree["locs"]))
    # do varible substitution for this tree here
    if type(config["trees"][tree]) == str or type(config["trees"][tree]) == unicode:
        treeStr = config["trees"][tree]
        if(args.verbose>5):
            print " Substituting $%s for tree %s" % (treeStr, tree)
        if(not config.has_key("variables") or not config["variables"].has_key(treeStr)):
            print " ERROR: no variable:  variables.%s for tree %s" % (treeStr, tree)
            sys.exit(1)
        config["trees"][tree] = config["variables"][treeStr]
    myconfig = config["trees"][tree]
    if(args.verbose>4):
        print " Config: %s" % (myconfig)
    # Process this tree
    if(len(myconfig)==0 or len(mytree["locs"])==0):
        if(args.verbose>2):
            print " No processing for %s - skipping" % (tree)
    else:
        only = None
        if myconfig.has_key("only"):
            only = set(myconfig["only"])
            if (len(only)==0) and (mytree["treeprefix"] != ""):
                thePool = "%spool.res" % (mytree["treeprefix"])
                if (thePool in itemset):
                    if(args.verbose>0):
                        print "Removing %s because tree %s is empty." % (thePool, tree)
                    remove.add(thePool)
        else:
            print "tree %s - no ONLY"
        for l in range(len(mytree["locs"])):
            loc = mytree["locs"][l]
            if (only is not None) and not loc in only:
                # REMOVE loc
                toRemove = "%s%s%s" % (mytree["treeprefix"], loc, mytree["extension"])
                if(args.verbose>6):
                    print "Queueing for removal: %s" % toRemove
                remove.add(toRemove)

def addTreeByType(tree, mytree):
    if(args.verbose>1):
        print "(considering %s): %s" % (tree, mytree)
    trees[tree] = mytree
    mytree["locs"]=[]
    for i in range(len(items)):
        item = items[i]
        if item.startswith(mytree["treeprefix"]) and item.endswith(mytree["extension"]):
            mytree["locs"].append(item[len(mytree["treeprefix"]):-4])
    # now, process
    queueForRemoval(tree)

addTreeByType("converters",{"treeprefix":"", "extension":".cnv"})
addTreeByType("stringprep",{"treeprefix":"", "extension":".spp"})
addTreeByType("translit",{"treeprefix":"translit/", "extension":".res"})
addTreeByType("brkfiles",{"treeprefix":"brkitr/", "extension":".brk"})
addTreeByType("brkdict",{"treeprefix":"brkitr/", "extension":"dict"})
addTreeByType("confusables",{"treeprefix":"", "extension":".cfu"})

for i in range(len(items)):
    item = items[i]
    if item.endswith(RES_INDX):
        treeprefix = item[0:item.rindex(RES_INDX)]
        tree = None
        if treeprefix == "":
            tree = "ROOT"
        else:
            tree = treeprefix[0:-1]
        if(args.verbose>6):
            print "procesing %s" % (tree)
        trees[tree] = { "extension": ".res", "treeprefix": treeprefix, "hasIndex": True }
        # read in the resource list for the tree
        treelistfile = os.path.join(args.tmpdir,"%s.lst" % tree)
        runcmd("iculslocs", "-i %s -N %s -T %s -l > %s" % (outfile, dataname, tree, treelistfile))
        fi = open(treelistfile, 'rb')
        treeitems = fi.readlines()
        trees[tree]["locs"] = [treeitems[i].strip() for i in range(len(treeitems))]
        fi.close()
        if(not config.has_key("trees") or not config["trees"].has_key(tree)):
            print " Warning: filter file %s does not mention trees.%s - will be kept as-is" % (args.filterfile, tree)
        else:
            queueForRemoval(tree)

def removeList(count=0):
    # don't allow "keep" items to creep in here.
    global remove
    remove = remove - keep
    if(count > 10):
        print "Giving up - %dth attempt at removal." % count
        sys.exit(1)
    if(args.verbose>1):
        print "%d items to remove - try #%d" % (len(remove),count)
    if(len(remove)>0):
        oldcount = len(remove)
        hackerrfile=os.path.join(args.tmpdir, "REMOVE.err")
        removefile = os.path.join(args.tmpdir, "REMOVE.lst")
        fi = open(removefile, 'wb')
        for i in remove:
            print >>fi, i
        fi.close()
        rc = runcmd("icupkg","-r %s %s 2> %s" %  (removefile,outfile,hackerrfile),True)
        if rc is not 0:
            if(args.verbose>5):
                print "## Damage control, trying to parse stderr from icupkg.."
            fi = open(hackerrfile, 'rb')
            erritems = fi.readlines()
            fi.close()
            #Item zone/zh_Hant_TW.res depends on missing item zone/zh_Hant.res
            pat = re.compile("""^Item ([^ ]+) depends on missing item ([^ ]+).*""")
            for i in range(len(erritems)):
                line = erritems[i].strip()
                m = pat.match(line)
                if m:
                    toDelete = m.group(1)
                    if(args.verbose > 5):
                        print "<< %s added to delete" % toDelete
                    remove.add(toDelete)
                else:
                    print "ERROR: could not match errline: %s" % line
                    sys.exit(1)
            if(args.verbose > 5):
                print " now %d items to remove" % len(remove)
            if(oldcount == len(remove)):
                print " ERROR: could not add any mor eitems to remove. Fail."
                sys.exit(1)
            removeList(count+1)

# fire it up
removeList(1)

# now, fixup res_index, one at a time
for tree in trees:
    # skip trees that don't have res_index
    if not trees[tree].has_key("hasIndex"):
        continue
    treebunddir = args.tmpdir
    if(trees[tree]["treeprefix"]):
        treebunddir = os.path.join(treebunddir, trees[tree]["treeprefix"])
    if not (os.path.isdir(treebunddir)):
        os.mkdir(treebunddir)
    treebundres = os.path.join(treebunddir,RES_INDX)
    treebundtxt = "%s.txt" % (treebundres[0:-4])
    runcmd("iculslocs", "-i %s -N %s -T %s -b %s" % (outfile, dataname, tree, treebundtxt))
    runcmd("genrb","-d %s -s %s res_index.txt" % (treebunddir, treebunddir))
    runcmd("icupkg","-s %s -a %s%s %s" % (args.tmpdir, trees[tree]["treeprefix"], RES_INDX, outfile))
