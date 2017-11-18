#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import os
import sys
import copy
from waflib import Context
from waflib import Options
class WurfOptions(Options.OptionsContext):
	def parse_args(self):
		extra_parser=copy.deepcopy(self.parser)
		optstrings=[x.get_opt_string()for x in extra_parser._get_all_options()]
		extra_opts=extra_parser.add_option_group('Extra options')
		for arg in sys.argv[1:]:
			if arg.startswith('--'):
				key=arg.split('=',1)[0]
				if key not in optstrings:
					if'='in arg:
						extra_opts.add_option(key,default=None,dest=key[2:])
					else:
						extra_opts.add_option(key,default=False,action='store_true',dest=key[2:])
		args=sys.argv[:]
		show_help='-h'in sys.argv or'--help'in sys.argv
		if show_help:
			if'-h'in args:args.remove('-h')
			if'--help'in args:args.remove('--help')
		(Options.options,leftover_args)=extra_parser.parse_args(args)
		ctx=Context.create_context('resolve')
		ctx.options=Options.options
		ctx.cmd='resolve'
		ctx.opt=self
		ctx.active_resolvers='configure'in sys.argv and not show_help
		try:
			ctx.execute()
		finally:
			ctx.finalize()
		super(WurfOptions,self).parse_args()
