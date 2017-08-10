top = '.'
out = 'build'
APPNAME = 'simple_simulator'
VERSION = '0.1'
    
def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.add_option('--kodo_rlnc_lib', dest='kodo_rlnc_lib', default='/usr/local/lib/', action='store', help='Path to kodo-rlnc library')
    opt.add_option('--coin_clp_lib', dest='coin_clp_lib', default='/usr/local/lib/', action='store', help='Path to coin-Clp library')
    

def configure(cfg):
    cfg.check_waf_version(mini='1.8.14')
    cfg.load('compiler_c compiler_cxx')
    
    cfg.env.append_value('CXXFLAGS', '-std=c++0x')
    cfg.env.append_value('CXXFLAGS', '-ftree-vectorize')
    cfg.env.append_value('CXXFLAGS', '-O2')
    cfg.env.append_value('CXXFLAGS', '-Wall')
    cfg.env.append_value('CXXFLAGS', '-fPIC')
    
    cfg.check(features='cxx cxxprogram', lib=['boost_chrono', 'boost_filesystem', 'boost_iostreams', 'boost_program_options', 'boost_system', 'boost_thread', 'boost_timer'], libpath=[cfg.options.kodo_rlnc_lib+'kodo-rlnc/kodo_build'], uselib_store='BOOST')
    cfg.check(features='cxx cxxprogram', lib=['Clp', 'ClpSolver', 'CoinUtils', 'Osi', 'OsiClp', 'OsiCommonTests'], cxxflags='-I '+ cfg.options.coin_clp_lib + 'include/coin',linkflags='-L '+ cfg.options.coin_clp_lib + 'lib', uselib_store='CLPSOLVER')        
    cfg.check(features='cxx cxxprogram', lib=['cpuid', 'fifi', 'fifi_no_dispatch', 'gauge', 'gtest', 'sak', 'tables'], libpath=[cfg.options.kodo_rlnc_lib+'kodo-rlnc/kodo_build'], uselib_store='KODO')    
    cfg.check(features='cxx cxxprogram', lib=['pthread','stdc++'], libpath=['/usr/lib/x86_64-linux-gnu'], uselib_store='OTHER')    
    
       
    cfg.env.append_value('INCLUDES', [cfg.options.kodo_rlnc_lib+'kodo-rlnc/src',
    cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/allocate-7c9f51/1.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/boost-abe3de/2.1.0',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/cpuid-4d8071/4.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/endian-30a816/3.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/fifi-8960fd/24.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/gauge-f88f90/10.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/gtest-5c3bfe/3.0.0',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/hex-4d9037/3.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/kodo-core-de4387/6.1.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/meta-768c5a/2.1.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/platform-bccd32/2.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/recycle-b2469b/2.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/sak-1bdcea/15.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/storage-20a331/2.1.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/stub-90d487/5.0.1/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/tables-c83c83/6.0.0/src',
	cfg.options.kodo_rlnc_lib+'kodo-rlnc/bundle_dependencies/waf-tools-51dff5/3.14.1',
	cfg.options.coin_clp_lib + 'include/coin'])
	

def build(bld):
    
    bld.program(features='c cxx cxxprogram', source='main.cpp', target='simple_simulator', includes=['.'], 
    	use=[
    		'BOOST', 'KODO', 'OTHER', 'CLPSOLVER',
    		'traffic-generator', 
    		'utils',
    		'nc-routing-rules',
    		'multicast-brr',  
    		'god-view-routing-rules',
    		'comm-net', 
    		'comm-node',
    		'graph',
    		'lp-solver',
    		'ccack',
    		'matrix-operations',
    		'matrix-echelon',    		    		
    		'galois-field',
    		'galois-field-element',
    		'galois-field-polynomial',
    		'test'
    	])
    bld.recurse('traffic utils routing-rules network lp-solver ccack galois-field test')
