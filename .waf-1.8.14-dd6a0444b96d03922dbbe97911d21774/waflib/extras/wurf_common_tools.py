#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import os
import sys
from waflib.Configure import conf
from waflib import Options
def _check_minimum_python_version(opt,major,minor):
	if sys.version_info[:2]<(major,minor):
		opt.fatal("Python version not supported: {0}, ""required minimum version: {1}.{2}".format(sys.version_info[:3],major,minor))
def options(opt):
	_check_minimum_python_version(opt,2,7)
	opt.load('wurf_resolve_context')
	opt.load('wurf_configure_output')
	opt.load('wurf_dependency_bundle')
	opt.load('wurf_standalone')
def resolve(ctx):
	if ctx.is_toplevel():
		ctx.load('wurf_dependency_bundle')
def configure(conf):
	if conf.is_toplevel():
		conf.env["stored_options"]=Options.options.__dict__.copy()
		conf.load('wurf_dependency_bundle')
def build(bld):
	if bld.is_toplevel():
		bld.load('wurf_dependency_bundle')
@conf
def is_toplevel(self):
	return self.srcnode==self.path
@conf
def get_tool_option(conf,option):
	current=Options.options.__dict__
	stored=conf.env.stored_options
	if option in current and current[option]!=None:
		return current[option]
	elif option in stored and stored[option]!=None:
		return stored[option]
	else:
		conf.fatal('Missing option: %s'%option)
@conf
def has_tool_option(conf,option):
	current=Options.options.__dict__
	stored=conf.env.stored_options
	if option in current and current[option]!=None:
		return True
	elif option in stored and stored[option]!=None:
		return True
	else:
		return False
