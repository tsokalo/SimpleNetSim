#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

try:
	from.import semver
except:
	import semver
import os
import hashlib
git_protocols=['https://','git@','git://']
git_protocol_handler=''
def options(opt):
	git_opts=opt.add_option_group('Git options')
	git_opts.add_option('--git-protocol',default=None,dest='git_protocol',help="Use a specific git protocol to download dependencies. ""Supported protocols: {0}".format(git_protocols))
	git_opts.add_option('--check-git-version',default=True,dest='check_git_version',help="Specifies if the minimum git version should be checked")
def resolve(ctx):
	ctx.load('wurf_git')
	if ctx.options.check_git_version:
		ctx.git_check_minimum_version((1,7,0))
	parent_url=None
	try:
		parent_url=ctx.git_config(['--get','remote.origin.url'],cwd=os.getcwd())
	except Exception ,e:
		ctx.to_log('Exception when executing git config - fallback to ''default protocol! parent_url: {0}'.format(parent_url))
		ctx.to_log(e)
	global git_protocol_handler
	if ctx.options.git_protocol:
		git_protocol_handler=ctx.options.git_protocol
	else:
		for g in git_protocols:
			if parent_url and parent_url.startswith(g):
				git_protocol_handler=g
				break
		else:
			git_protocol_handler='https://'
			from waflib.Logs import warn
			warn("Using default git protocol ({}) for dependencies. ""Use --git-protocol=[proto] to assign another protocol ""for dependencies. Supported protocols: {}".format(git_protocol_handler,git_protocols))
	if git_protocol_handler not in git_protocols:
		ctx.fatal('Unknown git protocol specified: "{}", supported protocols ''are {}'.format(git_protocol_handler,git_protocols))
class ResolveVersion(object):
	def __init__(self,name,git_repository,major,minor=None,patch=None):
		self.name=name
		self.git_repository=git_repository
		self.major=major
		self.minor=minor
		self.patch=patch
	def repository_url(self,ctx,protocol_handler):
		repo_url=self.git_repository
		if repo_url.count('://')>0 or repo_url.count('@')>0:
			ctx.fatal('Repository URL contains the following ''git protocol handler: {}'.format(repo_url))
		if protocol_handler not in git_protocols:
			ctx.fatal('Unknown git protocol specified: "{}", supported ''protocols are {}'.format(protocol_handler,git_protocols))
		if protocol_handler=='git@':
			if repo_url.startswith('github.com/'):
				repo_url=repo_url.replace('github.com/','github.com:',1)
			elif repo_url.startswith('bitbucket.org/'):
				repo_url=repo_url.replace('bitbucket.org/','bitbucket.org:',1)
			else:
				ctx.fatal('Unknown SSH host: {}'.format(repo_url))
		return protocol_handler+repo_url
	def resolve(self,ctx,path,use_checkout):
		path=os.path.abspath(os.path.expanduser(path))
		repo_url=self.repository_url(ctx,git_protocol_handler)
		repo_hash=hashlib.sha1(repo_url.encode('utf-8')).hexdigest()[:6]
		repo_folder=os.path.join(path,self.name+'-'+repo_hash)
		if not os.path.exists(repo_folder):
			ctx.to_log("Creating new repository folder: {}".format(repo_folder))
			os.makedirs(repo_folder)
		master_path=os.path.join(repo_folder,'master')
		if not os.path.isdir(master_path):
			ctx.git_clone(repo_url,master_path,cwd=repo_folder)
		else:
			try:
				ctx.git_pull(cwd=master_path)
			except Exception ,e:
				ctx.to_log('Exception when executing git pull:')
				ctx.to_log(e)
		ctx.git_get_submodules(master_path)
		if use_checkout:
			checkout_path=os.path.join(repo_folder,use_checkout)
			if use_checkout!='master':
				if not os.path.isdir(checkout_path):
					ctx.git_clone(repo_url,checkout_path,cwd=repo_folder)
					ctx.git_checkout(use_checkout,cwd=checkout_path)
				else:
					ctx.git_pull(cwd=checkout_path)
				ctx.git_get_submodules(checkout_path)
			tags=ctx.git_tags(cwd=checkout_path)
			for tag in tags:
				try:
					if semver.parse(tag)['major']>self.major:
						ctx.fatal("Tag {} in checkout {} is newer than the ""required major version {}".format(tag,use_checkout,self.major))
				except ValueError:
					pass
			return checkout_path
		tags=[]
		try:
			tags=ctx.git_tags(cwd=master_path)
		except Exception ,e:
			ctx.to_log('Exception when executing git tags:')
			ctx.to_log(e)
			tags=[d for d in os.listdir(repo_folder)if os.path.isdir(os.path.join(repo_folder,d))]
			ctx.to_log('Using the following fallback tags:')
			ctx.to_log(tags)
		if len(tags)==0:
			ctx.fatal("No version tags specified for {} - impossible to track ""version".format(self.name))
		tag=self.select_tag(tags)
		if not tag:
			ctx.fatal("No compatible tags found {} to track major version {} ""of {}".format(tags,self.major,self.name))
		tag_path=os.path.join(repo_folder,tag)
		if not os.path.isdir(tag_path):
			ctx.git_local_clone(master_path,tag_path,cwd=repo_folder)
			ctx.git_checkout(tag,cwd=tag_path)
			ctx.git_get_submodules(tag_path)
		return tag_path
	def select_tag(self,tags):
		valid_tags=[]
		for tag in tags:
			try:
				t=semver.parse(tag)
				if(self.major is not None and t['major']!=self.major)or(self.minor is not None and t['minor']!=self.minor)or(self.patch is not None and t['patch']!=self.patch):
					continue
				valid_tags.append(tag)
			except ValueError:
				pass
		if len(valid_tags)==0:
			return None
		best_match=valid_tags[0]
		for t in valid_tags:
			if semver.match(best_match,"<"+t):
				best_match=t
		return best_match
	def __eq__(self,other):
		s=(self.git_repository.lower(),self.major,self.minor,self.patch)
		o=(other.git_repository.lower(),other.major,other.minor,other.patch)
		return s==o
	def __ne__(self,other):
		return not self==other
	def __lt__(self,other):
		s=(self.git_repository.lower(),self.major,self.minor,self.patch)
		o=(other.git_repository.lower(),other.major,other.minor,other.patch)
		return s<o
	def __repr__(self):
		f=('ResolveVersion(name={}, git_repository={}, major={}, minor={}, ''patch={})')
		return f.format(self.name,self.git_repository,self.major,self.minor,self.patch)
