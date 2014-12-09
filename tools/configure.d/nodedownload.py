#!/usr/bin/env python
# Moved some utilities here from ../../configure

import urllib
import hashlib
import sys
import zipfile
import tarfile

def formatSize(amt):
    """Format a size as a string in MB"""
    return "{:.1f}".format(amt / 1024000.)

def spin(c):
    """print out a spinner based on 'c'"""
#    spin = "\\|/-"
    spin = ".:|'"
    return (spin[c % len(spin)])

class ConfigOpener(urllib.FancyURLopener):
    """fancy opener used by retrievefile. Set a UA"""
    # append to existing version (UA)
    version = '%s node.js/configure' % urllib.URLopener.version

def reporthook(count, size, total):
    """internal hook used by retrievefile"""
    sys.stdout.write(' Fetch: %c %sMB total, %sMB downloaded   \r' %
                     (spin(count),
                      formatSize(total),
                      formatSize(count*size)))

def retrievefile(url, targetfile):
    """fetch file 'url' as 'targetfile'. Return targetfile or throw."""
    try:
        sys.stdout.write(' <%s>\nConnecting...\r' % url)
        sys.stdout.flush()
        msg = ConfigOpener().retrieve(url, targetfile, reporthook=reporthook)
        print ''  # clear the line
        return targetfile
    except:
        print ' ** Error occurred while downloading\n <%s>' % url
        raise

def md5sum(targetfile):
    """md5sum a file. Return the hex digest."""
    digest = hashlib.md5()
    with open(targetfile, 'rb') as f:
      chunk = f.read(1024)
      while chunk !=  "":
        digest.update(chunk)
        chunk = f.read(1024)
    return digest.hexdigest()

def unpackWithMode(unpacker, packedfile, parent_path, mode):
    with unpacker.open(packedfile, mode) as icuzip:
        print ' Extracting source file: %s' % packedfile
        icuzip.extractall(parent_path)

def unpack(packedfile, parent_path):
    """Unpack packedfile into parent_path. Assumes .zip."""
    packedsuffix = packedfile.lower().split('.')[-1]  # .zip, .tgz etc
    if zipfile.is_zipfile(packedfile):
        unpackWithMode(zipfile.ZipFile, packedfile, parent_path, 'r')
    elif tarfile.is_tarfile(packedfile):
        # if (packedsuffix == 'tgz') or (packedsuffix == 'gz'):
        #     unpackWithMode(tarfile.TarFile, packedfile, parent_path, 'r:gz')
        # elif (packedsuffix == 'bz2') or (packedsuffix == 'tb2'):
        #     unpackWithMode(tarfile.TarFile, packedfile, parent_path, 'r:bz2')
        # else:
            unpackWithMode(tarfile.TarFile, packedfile, parent_path, 'r')
            #raise Exception('Error: Don\'t know how to unpack tarfile %s with extension %s' % (packedfile, packedsuffix))
    else:
        raise Exception('Error: Don\'t know how to unpack %s with extension %s' % (packedfile, packedsuffix))
