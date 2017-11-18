#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import os
from waflib import Utils
from waflib import Context
from waflib import Options
from waflib import Logs
from waflib.Configure import ConfigurationContext
class ResolveContext(ConfigurationContext):
	'''resolves the dependencies specified in the wscript's resolve function'''
	cmd='resolve'
	fun='resolve'
	def __init__(self,**kw):
		super(ResolveContext,self).__init__(**kw)
	def load(self,tool_list,*k,**kw):
		Context.Context.load(self,tool_list,*k,**kw)
	def execute(self):
		self.srcnode=self.path
		self.bldnode=self.path.make_node('build')
		self.bldnode.mkdir()
		if self.active_resolvers:
			path=os.path.join(self.bldnode.abspath(),'resolve.log')
			self.logger=Logs.make_logger(path,'cfg')
		Context.Context.execute(self)
		import waflib.extras.wurf_dependency_bundle as bundle
		bundle.post_resolve(self)
