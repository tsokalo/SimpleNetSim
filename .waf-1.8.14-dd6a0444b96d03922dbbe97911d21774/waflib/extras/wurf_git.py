#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import os
import re
from waflib.Configure import conf
from waflib import Utils
from waflib import Errors
try:
	import _winreg
except:
	try:
		import winreg as _winreg
	except:
		_winreg=None
def find_in_winreg():
	reg_value=reg_type=None
	try:
		reg_key=_winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE,'SOFTWARE\\Wow6432node\\Microsoft''\\Windows\\CurrentVersion\\''Uninstall\\Git_is1')
		(reg_value,reg_type)=_winreg.QueryValueEx(reg_key,'InstallLocation')
	except WindowsError:
		try:
			reg_key=_winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE,'SOFTWARE\\Microsoft''\\Windows\\CurrentVersion\\''Uninstall\\Git_is1')
			(reg_value,reg_type)=_winreg.QueryValueEx(reg_key,'InstallLocation')
		except WindowsError:
			pass
	if reg_type==_winreg.REG_SZ:
		return str(reg_value)
	else:
		return''
def find_git_win32(ctx):
	path=find_in_winreg()
	if path:
		path_list=[path,os.path.join(path,'bin')]
		ctx.find_program('git',path_list=path_list)
	else:
		ctx.find_program('git')
def resolve(ctx):
	if Utils.is_win32:
		find_git_win32(ctx)
	else:
		ctx.find_program('git')
@conf
def git_check_minimum_version(conf,minimum):
	version=conf.git_version()
	if version[:3]<minimum:
		conf.fatal("Git version not supported: {0}, ""required minimum version: {1}".format(version,minimum))
@conf
def git_cmd_and_log(ctx,args,**kw):
	if not'GIT'in ctx.env:
		raise Errors.WafError('The git program must be available')
	git_cmd=ctx.env['GIT']
	args=git_cmd+args
	return ctx.cmd_and_log(args,**kw)
@conf
def git_version(ctx,**kw):
	output=git_cmd_and_log(ctx,['version'],**kw).strip()
	int_list=[int(s)for s in re.findall('\\d+',output)]
	return tuple(int_list)
@conf
def git_tags(ctx,**kw):
	o=git_cmd_and_log(ctx,['tag','-l'],**kw)
	tags=o.split('\n')
	return[t for t in tags if t!='']
@conf
def git_checkout(ctx,branch,**kw):
	git_cmd_and_log(ctx,['checkout',branch],**kw)
@conf
def git_pull(ctx,**kw):
	git_cmd_and_log(ctx,['pull'],**kw)
@conf
def git_config(ctx,args,**kw):
	output=git_cmd_and_log(ctx,['config']+args,**kw)
	return output.strip()
@conf
def git_branch(ctx,**kw):
	o=git_cmd_and_log(ctx,['branch'],**kw)
	branch=o.split('\n')
	branch=[b for b in branch if b!='']
	current=''
	others=[]
	for b in branch:
		if b.startswith('*'):
			current=b[1:].strip()
		else:
			others.append(b)
	if current=='':
		ctx.fatal('Failed to locate current branch')
	return current,others
@conf
def git_get_submodules(ctx,repository_dir):
	if ctx.git_has_submodules(repository_dir):
		ctx.git_submodule_sync(cwd=repository_dir)
		ctx.git_submodule_init(cwd=repository_dir)
		ctx.git_submodule_update(cwd=repository_dir)
@conf
def git_has_submodules(ctx,repository_dir):
	return os.path.isfile(os.path.join(repository_dir,'.gitmodules'))
@conf
def git_submodule_init(ctx,**kw):
	git_cmd_and_log(ctx,['submodule','init'],**kw)
@conf
def git_submodule_update(ctx,**kw):
	git_cmd_and_log(ctx,['submodule','update'],**kw)
@conf
def git_submodule_sync(ctx,**kw):
	git_cmd_and_log(ctx,['submodule','sync'],**kw)
@conf
def git_clone(ctx,source,destination,**kw):
	git_cmd_and_log(ctx,['clone',source,destination],**kw)
@conf
def git_local_clone(ctx,source,destination,**kw):
	git_cmd_and_log(ctx,['clone','-l','--no-hardlinks',source,destination],**kw)
