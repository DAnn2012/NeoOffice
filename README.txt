Intructions for Building NeoOffice
----------------------------------

At this time, the NeoOffice build supports building on Mac OS X 10.4 (Tiger) or 10.5 (Leopard).


Steps for building on Mac OS X 10.4 (Tiger) or 10.5 (Leopard) for both PowerPC and Intel
----------------------------------------------------------------------------------------

1. Make sure that you have downloaded and installed the following dependencies from http://connect.apple.com/ website:

   For OS X 10.4 Tiger:
   
   Xcode Tools v2.5
   
   For OS X 10.5 Leopard:
   
   Xcode Tools v3.0

   Important: you will need to select the X11SDK package during installation as this package is required to build OpenOffice.org moz module.
   
2. If you are building under Mac OS X 10.4 Tiger, make sure that you have set the compiler version to the correct version by executing the following command:

   sudo gcc_select 4.0

3. Make sure that you have downloaded and installed the following Perl module from the http://www.cpan.org/modules/index.html website. Note that you will need to follow the instructions on the website to download and install the Archive::Zip module:

   Archive::Zip

4. Make sure that you have installed the "gcp" and "pkg-config" commands. You can download, compile, and install these commands by downloading, compiling, and installing the following packages from the http://www.macports.org/ website. Note that you will need to follow the instructions on the website for downloading, compiling, and installing the DarwinPorts "port" command. The "port" command is then used to do the downloading, compiling, and installation of the following packages:

   sudo /path/to/port/command install coreutils
   sudo /path/to/port/command install pkgconfig
   sudo /path/to/port/command install libIDL
   sudo /path/to/port/command install gperf
   sudo /path/to/port/command install flex

5. Make sure that you have downloaded and installed the Mono Mac OS X framework:

   http://www.go-mono.com/mono-downloads/download.html

   Important: version 2.4.x must be installed for the odf-converter code to build correctly. If you have more than one version of Mono installed, make sure that the /Library/Frameworks/Mono.framework/Versions softlink points to the correct Mono version folder.

6. Make sure that you have downloaded and installed the Subversion client and have the "svn" command in your PATH. Subversion binaries can be downloaded from here:

   http://subversion.tigris.org/project_packages.html

7. Start the build by invoking the following commands. Note that you should replace $NEO_HOME with absolute path of your workspace's "neojava" directory:

   cd $NEO_HOME
   make GNUCP=</absolute/path/of/your/gcp/command> LIBIDL_CONFIG=</absolute/path/of/your/libIDL-config-2/command> PKG_CONFIG=</absolute/path/of/your/pkg-config/command>

