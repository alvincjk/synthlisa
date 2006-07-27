#!/usr/bin/env python

from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc, get_python_lib
from distutils.dep_util import newer_group
from distutils.spawn import spawn

import sys
import os
import glob
import re

versiontag = '1.3.1'

synthlisa_prefix = ''
numeric_prefix = ''
swig_bin = 'swig'
gsl_prefix = ''

# At the moment, this setup script does not deal with --home.
# I should also modify the --help text to discuss these options

argv_replace = []

for arg in sys.argv:
    if arg.startswith('--prefix='):
        synthlisa_prefix = arg.split('=', 1)[1]
        argv_replace.append(arg)
    elif arg.startswith('--with-numeric='):
        numeric_prefix = arg.split('=', 1)[1]
    elif arg.startswith('--with-gsl='):
        gsl_prefix = arg.split('=', 1)[1]
    elif arg.startswith('--with-swig='):
        swig_bin = arg.split('=', 1)[1]
    else:
        argv_replace.append(arg)

sys.argv = argv_replace

gsl_cfiles = glob.glob('lisasim/GSL/*.c')
gsl_hfiles = glob.glob('lisasim/GSL/*.h')

lisasim_cppfiles = glob.glob('lisasim/*.cpp')
lisasim_hppfiles = glob.glob('lisasim/*.h')
lisasim_pyfiles  = glob.glob('lisasim/*.py')

# remove lisasim/lisasim-swig_wrap.h from headers

lisasim_hppfiles = filter(lambda s: s != 'lisasim/lisasim-swig_wrap.h',
                          lisasim_hppfiles)

source_files = lisasim_cppfiles + gsl_cfiles
header_files = lisasim_hppfiles + gsl_hfiles

# compile Id catalog for lisasim

idcatalog = ""

signature = re.compile('.*\$(Id.*)\$.*')

for file in source_files + header_files + lisasim_pyfiles:
    handle = open(file)

    for i in range(0,5):
        line = handle.readline()

        if signature.match(line):
            if idcatalog:
                idcatalog += '\n' + signature.match(line).group(1)
            else:
                idcatalog = signature.match(line).group(1)

    handle.close()

version_py = open('lisasim/version.py','w')
print >> version_py, "version_full = \"\"\"%s\"\"\"\n" % idcatalog
print >> version_py, "version_short = \"%s\"\n" % versiontag
version_py.close()

# Swig away!

lisasim_isource = 'lisasim/lisasim-swig.i'
lisasim_iheader = 'lisasim/lisasim-typemaps.i'

lisasim_cppfile = 'lisasim/lisasim-swig_wrap.cpp'
lisasim_pyfile = 'lisasim/lisaswig.py'

# Should I clean the swig-generated files when setup.py clean is issued?
# Better not, since it could engender problems if swig is not available.

def runswig(source,cppfile,pyfile,deps,cpp=1):
    if not os.path.isfile(cppfile) or not os.path.isfile(pyfile) \
           or newer_group(deps,cppfile) or newer_group(deps,pyfile):
        try:
            if cpp:
                spawn([swig_bin,'-w402','-c++','-python','-o',cppfile,source])
            else:
                spawn([swig_bin,'-w402','-python','-o',cppfile,source])
        except:
            print 'Sorry, I am unable to swig the modified ' + lisasim_isource
            sys.exit(1)

runswig(lisasim_isource,lisasim_cppfile,lisasim_pyfile,
        header_files + [lisasim_isource,lisasim_iheader])

if lisasim_cppfile not in source_files:
    source_files.append(lisasim_cppfile)

# Healpix

source_healpix = glob.glob('lisasim/healpix/*.cpp')
header_healpix = glob.glob('lisasim/healpix/*.h')

healpix_cppfile = 'lisasim/healpix/healpix_wrap.cpp'
healpix_pyfile = 'lisasim/healpix/healpix.py'

healpix_isource = 'lisasim/healpix/healpix.i'

runswig(healpix_isource,healpix_cppfile,healpix_pyfile,header_healpix)

if healpix_cppfile not in source_healpix:
    source_healpix.append(healpix_cppfile)

# Create the setdir.sh and setdir.csh scripts; they will be recreated
# each time setup.py is run, but it does not matter.

setdir_sh = open('lisasim/synthlisa-setdir.sh','w')
setdir_csh = open('lisasim/synthlisa-setdir.csh','w')

pythonpath = ''
installpath = sys.exec_prefix

if synthlisa_prefix:
    pythonpath = get_python_lib(prefix=synthlisa_prefix)
    installpath = synthlisa_prefix

if numeric_prefix:
    if pythonpath:
        pythonpath = pythonpath + ':'

    pythonpath = pythonpath + get_python_lib(prefix=numeric_prefix) + '/Numeric'

mpi_prefix = numeric_prefix

if mpi_prefix:
    if pythonpath:
        pythonpath = pythonpath + ':'
        
    pythonpath = pythonpath + get_python_lib(prefix=mpi_prefix) + '/mpi'

print >> setdir_sh, """
if [ -z "${PYTHONPATH}" ]
then
    PYTHONPATH="%s"; export PYTHONPATH
else
    PYTHONPATH="%s:$PYTHONPATH"; export PYTHONPATH
fi

SYNTHLISABASE="%s"; export SYNTHLISABASE
""" % (pythonpath, pythonpath, installpath)

print >> setdir_csh, """
if !($?PYTHONPATH) then
    setenv PYTHONPATH %s
else
    setenv PYTHONPATH %s:$PYTHONPATH
endif

setenv SYNTHLISABASE %s
""" % (pythonpath, pythonpath, installpath)

setdir_csh.close()
setdir_sh.close()

# Ready to setup, build, install!

if numeric_prefix:
    numeric_hfiles = get_python_inc(prefix=numeric_prefix)
else:
    numeric_hfiles = get_python_inc()

if pythonpath:
    setdir_scripts = ['lisasim/synthlisa-setdir.sh','lisasim/synthlisa-setdir.csh']
else:
    setdir_scripts = []

synthlisapackages = ['synthlisa', 'healpix']

synthlisapackage_dir = {'synthlisa' : 'lisasim','healpix' : 'lisasim/healpix'}

# do contribs (CPP only for the moment...)

contribs = []

for entry in glob.glob('contrib/*'):
    if os.path.isdir(entry):
        contrib_packagename = os.path.basename(entry)

        contrib_source_files = glob.glob(entry + '/*.cpp') + glob.glob(entry + '/*.cc')
        contrib_header_files = glob.glob(entry + '/*.h') + glob.glob(entry + '/*.hh')

        iscpp = 1

        if not contrib_source_files:
            # do C extension
            contrib_source_files = glob.glob(entry + '/*.c')
            iscpp = 0

        contrib_swigfiles = glob.glob(entry + '/*.i')

        extensions = 0

        # each SWIG file creates a separate extension
        for contrib_swigfile in contrib_swigfiles:

            # let's check if GSL is needed
            gsl_required = 0

            for line in open(contrib_swigfile).readlines():
                if 'requires GSL' in line:
                    # the magic words are "requires GSL"
                    gsl_required = 1

            if gsl_required == 1 and gsl_prefix == '':
                print "No GSL, skipping " + contrib_swigfile
                break

            contrib_basefile = re.sub('\.i$','',contrib_swigfile)
            contrib_basename = os.path.basename(contrib_basefile)
    
            # assume SWIG file has the same name of the module
            contrib_wrapfile = contrib_basefile + '_wrap.cpp'
            contrib_pyfile = contrib_basefile + '.py'
    
            runswig(contrib_swigfile,contrib_wrapfile,contrib_pyfile,
                    contrib_header_files + [contrib_swigfile],iscpp)
    
            contrib_extname = contrib_packagename + '/_' + contrib_basename

            if not contrib_wrapfile in contrib_source_files: 
                contrib_source_files.append(contrib_wrapfile)

            if gsl_required == 1:
                contrib_extension = Extension(contrib_extname,
                                              contrib_source_files,
                                              include_dirs = [entry,numeric_hfiles,gsl_prefix + '/include'],
                                              library_dirs = [gsl_prefix + '/lib'],
                                              runtime_library_dirs = [gsl_prefix + '/lib'],
                                              libraries=['gsl', 'gslcblas'],
                                              depends = contrib_header_files)            
            else:
                contrib_extension = Extension(contrib_extname,
                                              contrib_source_files,
                                              include_dirs = [entry,numeric_hfiles],
                                              depends = contrib_header_files)
    
            contribs.append(contrib_extension)
            
            extensions = extensions + 1

        if extensions:
            synthlisapackages.append(contrib_packagename)
            synthlisapackage_dir[contrib_packagename] = entry

# do the actual setup

setup(name = 'synthLISA',
      version = versiontag,
      description = 'Synthetic LISA Simulator',
      long_description = idcatalog,

      author = 'Michele Vallisneri',
      author_email = 'vallis@vallis.org',
      url = 'http://www.vallis.org/syntheticlisa',

      packages = synthlisapackages,
      
      package_dir = synthlisapackage_dir,

      scripts = setdir_scripts,

      data_files = [('share/synthlisa',['data/positions.txt',
                                        'data/lisa-xml.dtd',
                                        'data/lisa-xml.xsl',
                                        'data/lisa-xml.css'])],

      ext_modules = [Extension('synthlisa/_lisaswig',
                               source_files,
                               include_dirs = [numeric_hfiles],
                               depends = header_files
                               ),
                     Extension('healpix/_healpix',
                               source_healpix,
                               include_dirs = ['lisasim/healpix',numeric_hfiles],
                               depends = header_healpix
                               )] + contribs
      )
