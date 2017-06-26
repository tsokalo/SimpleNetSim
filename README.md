### How do I get set up? ###
(for Ubuntu)

1. COIN-CLP: clone it from https://github.com/coin-or/Clp. Run "./configure" and "make install" from the coin-Clp folder. Then, modify the path to your coin-clp installation in the wscript in the root of the SimpleNetSim folder.

2. KODO and BOOST: Clone the repository [kodo-rlnc] from https://github.com/steinwurf/kodo-rlnc. You need to apply for the academic license (http://steinwurf.com/license.html). Then, inside of the kodo-rlnc folder run "CXXFLAG="-fPIC" ./waf configure && ./waf build && ./waf install --install_static_libs --install_path="./kodo_build".

3. SIMPLENETSIM: "./waf configure && ./waf build"
