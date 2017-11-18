#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import os
import sys
import argparse
from waflib.Configure import conf
from waflib import Utils
from waflib import Logs
from waflib import Errors
from waflib import ConfigSet
OPTIONS_NAME='Dependency options'
DEFAULT_BUNDLE_PATH='bundle_dependencies'
DEPENDENCY_PATH_KEY='%s-path'
DEPENDENCY_CHECKOUT_KEY='%s-use-checkout'
dependencies=dict()
dependency_dict=dict()
dependency_list=[]
@conf
def add_dependency(ctx,resolver,recursive_resolve=True,optional=False):
	name=resolver.name
	if len(dependencies)==0 and name!='waf-tools':
		ctx.fatal('waf-tools should be added before other dependencies')
	if name in dependencies:
		if type(resolver)!=type(dependencies[name])or dependencies[name]!=resolver:
			ctx.fatal('Incompatible dependency resolvers %r <=> %r '%(resolver,dependencies[name]))
	if name not in dependencies:
		dependencies[name]=resolver
		bundle_opts=ctx.opt.get_option_group(OPTIONS_NAME)
		add=bundle_opts.add_option
		add('--%s-path'%name,dest=DEPENDENCY_PATH_KEY%name,default=None,help='Path to %s'%name)
		add('--%s-use-checkout'%name,dest=DEPENDENCY_CHECKOUT_KEY%name,default=None,help='The checkout to use for %s'%name)
		if ctx.active_resolvers:
			path=resolve_dependency(ctx,name,optional)
			if recursive_resolve and path:
				ctx.recurse([path])
		elif name in ctx.env['DEPENDENCY_DICT']:
			path=ctx.env['DEPENDENCY_DICT'][name]
			if recursive_resolve:
				ctx.recurse([path])
def expand_path(path):
	return os.path.abspath(os.path.expanduser(path))
def options(opt):
	opt.load('wurf_dependency_resolve')
	bundle_opts=opt.add_option_group(OPTIONS_NAME)
	add=bundle_opts.add_option
	add('--bundle-path',default=DEFAULT_BUNDLE_PATH,dest='bundle_path',help='The folder where the bundled dependencies are downloaded. ''Default folder: "{}"'.format(DEFAULT_BUNDLE_PATH))
def resolve_dependency(ctx,name,optional=False):
	p=argparse.ArgumentParser()
	p.add_argument('--%s-path'%name,dest='dependency_path',type=str)
	args,unknown=p.parse_known_args(args=sys.argv[1:])
	dependency_path=args.dependency_path
	if dependency_path:
		dependency_path=expand_path(dependency_path)
		ctx.start_msg('User resolve dependency %s'%name)
		ctx.end_msg(dependency_path)
	else:
		bundle_path=expand_path(ctx.options.bundle_path)
		Utils.check_dir(bundle_path)
		ctx.start_msg('Resolve dependency %s'%name)
		p=argparse.ArgumentParser()
		p.add_argument('--%s-use-checkout'%name,dest='dependency_checkout',type=str)
		args,unknown=p.parse_known_args(args=sys.argv[1:])
		dependency_checkout=args.dependency_checkout
		try:
			dependency_path=dependencies[name].resolve(ctx=ctx,path=bundle_path,use_checkout=dependency_checkout)
		except Exception ,e:
			ctx.to_log('Exception when resolving dependency: {}'.format(name))
			ctx.to_log(e)
			if optional:
				ctx.end_msg('Unavailable',color='RED')
			else:
				repo_url=dependencies[name].repository_url(ctx,'https://')
				ctx.fatal('Error: the "{}" dependency is not available. ''Please check that you have a valid Steinwurf ''license and you can access the repository at: ''{}'.format(name,repo_url))
		else:
			ctx.end_msg(dependency_path)
	if dependency_path:
		ctx.env['DEPENDENCY_DICT'][name]=dependency_path
		dependency_list.append(dependency_path)
	return dependency_path
def resolve(ctx):
	if ctx.active_resolvers:
		ctx.load('wurf_dependency_resolve')
		ctx.env['DEPENDENCY_DICT']=dict()
	else:
		try:
			path=os.path.join(ctx.bldnode.abspath(),'resolve.config.py')
			ctx.env=ConfigSet.ConfigSet(path)
		except EnvironmentError:
			pass
def post_resolve(ctx):
	if ctx.active_resolvers:
		dependency_dict.update(ctx.env['DEPENDENCY_DICT'])
		path=os.path.join(ctx.bldnode.abspath(),'resolve.config.py')
		ctx.env.store(path)
def configure(conf):
	conf.env['DEPENDENCY_LIST']=dependency_list
	conf.env['DEPENDENCY_DICT']=dependency_dict
	for path in conf.env['DEPENDENCY_LIST']:
		conf.recurse([path])
def build(bld):
	for path in reversed(bld.env['DEPENDENCY_LIST']):
		bld.recurse([path])
@conf
def has_dependency_path(self,name):
	return name in self.env['DEPENDENCY_DICT']
@conf
def dependency_path(self,name):
	return self.env['DEPENDENCY_DICT'][name]
