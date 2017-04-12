top = '.'
out = 'build'
APPNAME = 'simple_simulator'
VERSION = '0.1'
    
def options(opt):
    opt.load('compiler_c compiler_cxx')
    
 

def configure(cfg):
    cfg.check_waf_version(mini='1.8.14')
    cfg.load('compiler_c compiler_cxx')
    
    cfg.env.append_value('CXXFLAGS', '-std=c++0x')
    cfg.env.append_value('CXXFLAGS', '-ftree-vectorize')
    cfg.env.append_value('CXXFLAGS', '-O2')
    cfg.env.append_value('CXXFLAGS', '-Wall')
    cfg.env.append_value('CXXFLAGS', '-fPIC')
    
    cfg.check(features='cxx cxxprogram', lib=['boost_chrono', 'boost_filesystem', 'boost_iostreams', 'boost_program_options', 'boost_system', 'boost_thread', 'boost_timer'], libpath=['/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/kodo_build'], uselib_store='BOOST')
    cfg.check(features='cxx cxxprogram', lib=['Clp', 'ClpSolver', 'CoinUtils', 'Osi', 'OsiClp', 'OsiCommonTests'], cxxflags='-I /home/tsokalo/workspace/coin-Clp/include/coin',linkflags='-L /home/tsokalo/workspace/coin-Clp/lib', uselib_store='CLPSOLVER')        
    cfg.check(features='cxx cxxprogram', lib=['cpuid', 'fifi', 'fifi_no_dispatch', 'gauge', 'gtest', 'sak', 'tables'], libpath=['/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/kodo_build'], uselib_store='KODO')    
    cfg.check(features='cxx cxxprogram', lib=['pthread','stdc++'], libpath=['/usr/lib/x86_64-linux-gnu'], uselib_store='OTHER')    
    
       
    cfg.env.append_value('INCLUDES', ['/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/src',
    '/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/allocate-7c9f51/1.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/boost-abe3de/2.1.0',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/cpuid-4d8071/4.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/endian-30a816/3.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/fifi-8960fd/24.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/gauge-f88f90/10.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/gtest-5c3bfe/3.0.0',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/hex-4d9037/3.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/kodo-core-de4387/6.1.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/meta-768c5a/2.1.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/platform-bccd32/2.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/recycle-b2469b/2.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/sak-1bdcea/15.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/storage-20a331/2.1.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/stub-90d487/5.0.1/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/tables-c83c83/6.0.0/src',
	'/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/waf-tools-51dff5/3.14.1',
	'/home/tsokalo/workspace/tmaTool/coin-Clp/include/coin'])
	

def build(bld):
    
    bld.program(features='c cxx cxxprogram', source='main.cpp', target='simple_simulator', includes=['.'], 
    	use=[
    		'BOOST', 'KODO', 'OTHER', 'CLPSOLVER',
    		'traffic/traffic-generator', 
    		'utils/utils',
    		'routing-rules/nc-routing-rules', 
    		'routing-rules/god-view-routing-rules',
    		'network/comm-net', 
    		'network/comm-node',
    		'lp-solver/graph',
    		'lp-solver/lp-solver',
    		'ccack/ccack',
    		'ccack/matrix-operations',
    		'ccack/matrix-echelon',    		    		
    		'galois-field/galois-field',
    		'galois-field/galois-field-element',
    		'galois-field/galois-field-polynomial',
    		'test/test'
    	])
    bld.recurse('traffic utils routing-rules network lp-solver ccack galois-field test')
