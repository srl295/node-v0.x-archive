# Moved some utilities here from ../../configure

def formatSize(amt):
    """Format a size as a string"""
    return "{:.1f}".format(amt / 1024000.)

def spin(c):
    """print out a spinner based on 'c'"""
#    spin = "\\|/-"
    spin = ".:|'"
    return (spin[c % len(spin)])
