### How do I get set up? ###
(for Ubuntu)

1. COIN-CLP: clone it from https://github.com/coin-or/Clp. Run "./configure" and "make install" from the coin-Clp folder.

2. KODO: Clone the repository [kodo-rlnc] from https://github.com/steinwurf/kodo-rlnc. You need to apply for the academic license (http://steinwurf.com/license.html). Then, inside of the kodo-rlnc folder run "CXXFLAG="-fPIC" ./waf configure && ./waf build && ./waf install --install_static_libs --install_path="./kodo_build".

3. SIMPLENETSIM: 
	- clone this repository; 
	- run "git update-index --assume-unchanged build.conf"; 
	- edit in build.conf the paths to COIN-CLP and KODO; 
	- run "./configure.sh"; run "./install.sh"; run "./run.sh 0"
