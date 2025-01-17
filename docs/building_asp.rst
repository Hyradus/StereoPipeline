.. _building_asp:

Building ASP
============

This chapter will describe how ASP can be built without and with using
conda, and how to build the documentation.

.. _build_from_source:

Building ASP without conda
--------------------------

This entails downloading all the ASP dependencies with conda first as
pre-compiled binaries, then pulling the VisionWorkbench and Stereo
Pipeline source code from GitHub, and building locally. This is
suggested only for the very adventurous user.

The environments having the ASP dependencies are in the ``conda``
directory of the Stereo Pipeline repository, as above. After
downloading those, one can run on Linux::

    conda env create -f asp_deps_3.2.0_linux_env.yaml

or on the Mac::

    conda env create -f asp_deps_3.2.0_osx_env.yaml

This will create an ``asp_deps`` environment. Activate it with::

    conda activate asp_deps

Some of the .la files created by conda point to other .la files that
are not available. For that reason, those files should be edited to
replace::

    /path/to/libmylibrary.la

with::

    -L/path/to -lmylibrary

This can be done with the following commands::

    cd ~/miniconda3/envs/asp_deps/lib
    mkdir -p  backup
    cp -fv  *.la backup # back these up
    perl -pi -e "s#(/[^\s]*?lib)/lib([^\s]+).la#-L\$1 -l\$2#g" *.la

The `conda-provided compilers
<https://conda.io/projects/conda-build/en/latest/resources/compiler-tools.html>`_
are used and should be installed in the environment, if not present already.

Also ensure that ``cmake>=3.15.5`` and ``pbzip2`` are installed, and,
for Linux only, the ``chrpath`` tool.

Set up a work directory::

    buildDir=$HOME/build_asp
    mkdir -p $buildDir

Set up the compiler, on Linux and OSX::

    isMac=$(uname -s | grep Darwin)
    if [ "$isMac" != "" ]; then
      cc_comp=clang
      cxx_comp=clang++
    else
      cc_comp=x86_64-conda_cos6-linux-gnu-gcc
      cxx_comp=x86_64-conda_cos6-linux-gnu-g++
  fi

Build VisionWorkbench and Stereo Pipeline::

    cd $buildDir
    envPath=$HOME/miniconda3/envs/asp_deps
    $envPath/bin/git clone                            \
        git@github.com:visionworkbench/visionworkbench.git
    cd visionworkbench
    # Uncomment below if desired to build a specific version
    # git checkout 3.2.0
    mkdir -p build
    cd build
    $envPath/bin/cmake ..                             \
      -DASP_DEPS_DIR=$envPath                         \
      -DCMAKE_VERBOSE_MAKEFILE=ON                     \
      -DCMAKE_INSTALL_PREFIX=$buildDir/install        \
      -DCMAKE_C_COMPILER=${envPath}/bin/$cc_comp      \
      -DCMAKE_CXX_COMPILER=${envPath}/bin/$cxx_comp
    make -j10 && make install

    cd $buildDir
    envPath=$HOME/miniconda3/envs/asp_deps
    $envPath/bin/git clone                            \
    git@github.com:NeoGeographyToolkit/StereoPipeline.git
    cd StereoPipeline
    # Uncomment below if desired to build a specific version
    # git checkout 3.2.0
    mkdir -p build
    cd build
    $envPath/bin/cmake ..                             \
      -DASP_DEPS_DIR=$envPath                         \
      -DCMAKE_VERBOSE_MAKEFILE=ON                     \
      -DCMAKE_INSTALL_PREFIX=$buildDir/install        \
      -DVISIONWORKBENCH_INSTALL_DIR=$buildDir/install \
      -DCMAKE_C_COMPILER=${envPath}/bin/$cc_comp      \
      -DCMAKE_CXX_COMPILER=${envPath}/bin/$cxx_comp
    make -j10 && make install

.. _conda_build:

Building ASP and its dependencies with conda
--------------------------------------------

This page is meant for advanced users of ASP and maintainers who would
like to use conda to rebuild ASP and all its dependencies. It is
suggested to carefully read :numref:`conda_intro` before this page.

To simplify maintenance, ASP and its dependencies are built upon ISIS
and its dependencies. Hence, in order to create a new conda ASP
package, first one needs to create an environment having the latest
released ISIS, then rebuild ASP's other dependencies and ASP itself,
while ensuring that the dependencies of each of these have their
versions synced up with the ISIS dependency versions.

The rebuilt packages will be uploaded to ASP's anaconda channel.

Setting up the ISIS environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Search for the latest available ISIS conda package::
  
    conda search -c usgs-astrogeology --override-channels isis

Here it was found that ISIS version 6 was the latest, which we
will assume throughout the rest of this document. This needs to be
adjusted for your circumstances.

Create a conda environment for this version of ISIS::

     conda create -n isis7.1.0
     conda activate isis7.1.0

Add these channels to conda::

    conda config --env --add channels conda-forge
    conda config --env --add channels usgs-astrogeology

Run::

    conda config --show channels

and verify that ``usgs-astrogeology`` and ``conda-forge`` are in this
order and above all other channels, except perhaps the
``nasa-ames-stereo-pipeline`` channel.

Install the desired version of ISIS::

    conda install isis==7.1.0

Search and install the latest version of the ``usgscsm`` package,
for example, as::

    conda search -c conda-forge --override-channels usgscsm
    conda install -c conda-forge usgscsm==1.6.0

If that package is too old, consider rebuilding it, following
the recipe at:

    https://github.com/NeoGeographyToolkit/usgscsm-feedstock

See :numref:`packages_to_build` for how to fetch and build this.
  
Save the current environment as follows::

    conda env export > isis7.1.0.yaml

Fetching the build tools
~~~~~~~~~~~~~~~~~~~~~~~~

We will create a new ``tools`` environment to have all the tools we
will need. These could be appended to the earlier environment, but it
is less likely to to have issues with dependency conflicts if these
are kept separate.

::

    conda create -n tools
    conda activate tools
    conda install -c conda-forge anaconda-client conda-build \
      conda-verify

.. _packages_to_build:

Packages to build
~~~~~~~~~~~~~~~~~

The additional packages that need to be built can be fetched with ``git
clone`` from:

  https://github.com/NeoGeographyToolkit/geoid-feedstock.git
  https://github.com/NeoGeographyToolkit/htdp-feedstock.git
  https://github.com/NeoGeographyToolkit/laszip-feedstock.git
  https://github.com/NeoGeographyToolkit/fgr-feedstock.git
  https://github.com/NeoGeographyToolkit/libnabo-feedstock.git
  https://github.com/NeoGeographyToolkit/libpointmatcher-feedstock.git
  https://github.com/NeoGeographyToolkit/gdal-feedstock.git
  https://github.com/NeoGeographyToolkit/liblas-feedstock.git
  https://github.com/NeoGeographyToolkit/s2p-feedstock.git
  https://github.com/NeoGeographyToolkit/libelas-feedstock.git
  https://github.com/NeoGeographyToolkit/multiview-feedstock
  https://github.com/NeoGeographyToolkit/visionworkbench-feedstock.git
  https://github.com/NeoGeographyToolkit/stereopipeline-feedstock.git

Also, per the earlier note, consider rebuilding ``usgscsm`` if
there there are updates in its GitHub repository which are not yet
released on conda-forge.

Synchronize the versions with the existing environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For each of the above feedstocks, check the ``recipe/meta.yaml`` file
and ensure all dependencies are in sync with what is in the file
``isis7.1.0.yaml`` generated earlier. This can be done automatically
with a provided script in the ASP repository::

     python StereoPipeline/conda/update_versions.py isis7.1.0.yaml \
       gdal-feedstock

and the same for the other packages.

It is very important to note that this script is not fool-proof, and the
changes it makes should be very carefully examined.

It is suggested to examine the changed ``meta.yaml``, and if in doubt,
leave the values as they were before modified by this script. In each
of those files manually modify the string ``isis7.1.0`` to reflect the
current ISIS version.

In the ``visionworkbench`` and ``stereopipeline`` recipes update the
``git_tag`` value to reflect the desired commit from the Git
history. (When making an ASP release, one can tag the commit based on
which the release happens in the VisionWorkbench and StereoPipeline
repositories, and then that tag can be used in the ``git_tag`` field.)

Later on, after the packages are built and tested, ensure that all the
changes to the feedstock repositories are checked in.

Build the conda packages
~~~~~~~~~~~~~~~~~~~~~~~~

Each of the packages above can be built as follows::

    conda build -c nasa-ames-stereo-pipeline -c usgs-astrogeology \
      -c conda-forge gdal-feedstock

(Consider using the options ``--no-verify --no-test`` with this tool
if it fails with with unrelated errors at the packaging stage, as
it happened on OSX on occasion. This is a risky option and should
be a measure of last resort.)

Upload it to the ``nasa-ames-stereo-pipeline`` channel by
first logging in, via the command:

::
    
    anaconda login

and specifying the channel as the user name, and then running a
command along the lines:

::

    anaconda upload \
      $HOME/miniconda3/envs/asp_deps/conda-bld/linux-64/mypackage.tar.bz2

(Use above the path echoed on the screen by the ``conda build``
command.)

Use the ``--force`` option if desired to overwrite any existing
package with the same name and version.

After a package is uploaded, it can be installed in the existing
``isis7.1.0`` environment as::

    conda install -c nasa-ames-stereo-pipeline \
      -c usgs-astrogeology                     \
      -c conda-forge                           \
      gdal==3.5_isis7

To list all packages in that channel, do::

    conda search -c nasa-ames-stereo-pipeline --override-channels

To delete a package from this channel, run::

    anaconda remove nasa-ames-stereo-pipeline/mypackage/myversion
  
Order of building the packages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is suggested to build the above packages in the order listed
earlier, as some of them depend on others.

Note that ``libpointmatcher`` depends on ``libnabo``, while ``liblas``
depends on ``laszip`` and ``gdal``, ``theia`` depends on
``imagemagick``, and ``visionworkbench`` depends on ``gdal``. The
``stereopipeline`` package depends on all of these so it should be
built the last.

Additional ASP dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~

VisionWorkbench and StereoPipeline have a few more conda dependencies
that need to be fetched from ``conda-forge``.

If desired to create an environemnt in which to build ASP or to update
the one in :numref:`build_from_source`, the dependencies can be looked
up in the ``meta.yaml`` files for these conda packages, after fetching
them according to :numref:`packages_to_build`.

Building the documentation
--------------------------

The ASP documentation is encoded in ReStructured Text and is built
with the Sphinx-Doc system (https://www.sphinx-doc.org) with 
sphinxcontrib-bibtex (https://sphinxcontrib-bibtex.readthedocs.io).
These packages can be installed and activated as follows::

    conda create -n sphinx -c conda-forge python=3.6 \
      sphinx=3.5.4 sphinxcontrib-bibtex=2.1.4  
    conda activate sphinx

Note that we used a separate conda environment to minimize the chance
of conflict with other dependencies. Also, Sphinx version 4 seems to
have trouble compiling our documentation, hence a lower version is
used here.

In order to build the PDF (but not the HTML) document, a full
LaTeX distribution is also necessary, such as TeX Live. 

The ``docs`` directory contains the root of the documentation. Running
``make html`` and ``make latexpdf`` there will create the HTML and PDF
versions of the documentation in the _build subdirectory. In
particular, the PDF document will be at::

  ./_build/latex/asp_book.pdf

