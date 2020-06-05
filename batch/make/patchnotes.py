#!/usr/bin/python3
#wd=`dirname $0`

# generates patch notes from git log references to gitlab issues
# parameters: <path to git repository> <path to frozen changelog> <gitlab team> <gitlab project>


# issue query URI sample
# https://gitlab.com/api/v4/projects/9837210/issues?scope=all&state=closed&iids[]=11&iids[]=9

# Looks for commit lines of the form
# Implements https://gitlab.com/armagetronad/armagetronad/-/issues/11
# or 
# Fixes #11
# looks up the referenced issues in the GitLab API, generates a ChangeLog/Patch Note line for each

import sys, argparse
import subprocess
from packaging import version
from string import Template
import urllib.request
import json

# checks whether s represents an iteger
def RepresentsInt(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

# retrieve tags from git repository
def GetTags(repo, tag_lower_limit):
	alltags_raw=subprocess.run(["git", "-C", repo, "tag", "-l", "--merged"], stdout=subprocess.PIPE)
	alltags=alltags_raw.stdout.decode('utf-8').split('\n')
	return list(filter(lambda x: len(x) > 0 and version.parse(x) >= version.parse(tag_lower_limit), alltags))

	# sort tags in chronological order (assuming they're all on the same branch)
	revisions={}
	for tag in tags:
		# count number of revisions since tag
		revisions_since_tag_raw=subprocess.run(["git", "-C", repo, "rev-list", "--count", tag + ".."], stdout=subprocess.PIPE)
		revisions[tag]=int(revisions_since_tag_raw.stdout.decode('utf-8'))

	print(revisions)
	tags.sort(key=lambda x: revisions[x], reverse=True)
	return tags

# return (tag, list of issues fixed after tag as integers)
def FixedAfterTag(repo, team, project, tags):
	# for each tag, collect which issues have been resolved (Fix.. Close.. Solve.. Implement...)
	# or mentioned otherwise
	uri_start=Template('https://gitlab.com/${team}/${project}/-/issues/').substitute(team=team, project=project)
	# The build system is on Python 3.5, so we're stuck with this mechanism
	# print(uri_start)

	last_fixed_in={}
	last_mentioned_in={}
	issues=set([])

	for tag in tags:
		log_raw=subprocess.run(["git", "-C", repo, "log", tag + ".."], stdout=subprocess.PIPE)
		log=log_raw.stdout.decode('utf-8').split('\n')

		for logline in log:
			mentions=False
			fixes=False
			ref=None
			if logline[0:4] == '    ':
				for word in logline.split():
					if word.startswith(uri_start):
						ref = word[len(uri_start):]
					elif word.startswith('#'):
						ref = word[1:]
					if not ref is None and RepresentsInt(ref):
						ref=int(ref)
						mentions = True
						trimmed=logline.strip().upper()
						if trimmed.startswith('FIX') or trimmed.startswith('IMPLEMENT') or trimmed.startswith('CLOSE') or trimmed.startswith('SOLVE'):
							fixes = True						
			if fixes:
				last_fixed_in[ref] = tag
			if mentions:
				last_mentioned_in[ref] = tag
				issues.add(ref)

	#print(issues)
	#print(last_fixed_in)
	#print(last_mentioned_in)

	fixed_after_tag={}
	for issue in issues:
		fixed_in = None
		if issue in last_fixed_in:
			fixed_in = last_fixed_in[issue]
		if fixed_in == None and issue in last_mentioned_in:
			fixed_in = last_mentioned_in[issue]
		if fixed_in != None:
			#print(issue, fixed_in)
			fixed_after_tag.setdefault(fixed_in, []).append(issue)

	#print(fixed_after_tag)
	return fixed_after_tag

# retrieves metadata for an issue from gitlab, composes markup patch note line
def GetMarkupLine(team, project, issue):
	uri=Template('https://gitlab.com/api/v4/projects/${team}%2F${project}/issues?scope=all&state=closed&iids[]=${issue}').substitute(team=team, project=project, issue=issue)
	#print(uri)
	with urllib.request.urlopen(uri) as content:
		data = json.loads(content.read().decode())
		#print(data)
		if len(data) == 0:
			return "", None
		title = data[0]['title']
		weblink = data[0]['web_url']
		labels = data[0]['labels']
		line = None
		if data[0]['state'] == 'closed':
			line = Template(' * ${title} ([#${issue}](${weblink}))').substitute(title=title.strip(), issue=issue, weblink=weblink)
		#print(line, labels)
		if 'Type::Bug' in labels:
			return 'Fixed Bugs', line
		elif 'Type::Feature' in labels:
			return 'New Features', line
		elif 'Type::Removed' in labels:
			return 'Removed', line
		elif 'Type::Breaking' in labels:
			return 'Breaking Changes', line
		else:
			return 'Other Changes', line

# parse given frozen changelog, look for last 'changes since' note, extract tag name
def GetLastFrozenTag(frozen):
	file = open(frozen, 'r')
	begin = '#### Changes since '
	for line in file:
		if line.startswith(begin):
			version=line[len(begin):]
			version = version.strip(':\n ')
			if len(version) > 2:
				if version[0] != 'v':
					version = 'v' + version
			return version

parser = argparse.ArgumentParser(description='Generates ChangeLog/Patch Notes')
parser.add_argument('repository', help='Path to the local git repository')
parser.add_argument('frozen_changelog', help='Path to a previous, now immutable changelog')
parser.add_argument('gitlab_team', help='Team name on gitlab')
parser.add_argument('gitlab_project', help='Project name on gitlab')
parser.add_argument('-p', help='Just print patch notes', action='store_true')

args = parser.parse_args()

if len(sys.argv) < 5:
	print("Not enough arguments")
	exit(1)

patchnotes = args.p
repo = args.repository
frozen = args.frozen_changelog
team = args.gitlab_team
project = args.gitlab_project

# adapt for other branches
tag_lower_limit=GetLastFrozenTag(frozen)
# print(tag_lower_limit)

# get all tags relevant to the current branch
tags=GetTags(repo, tag_lower_limit)
#print("tags =", tags)

fixed_after_tag=FixedAfterTag(repo, team, project, tags)
#print(fixed_after_tag)

for tag in fixed_after_tag:
	# print(tag)
	fixed = fixed_after_tag[tag]
	# luckily, the category names are alphabetically in the order we want them in :)
	categories={}
	#categories={'Fixed Bugs' : [], 'New Features' : [], 'Other Changes': []}
	for issue in fixed:
		category, line=GetMarkupLine(team, project, issue)
		if not line is None:
			categories.setdefault(category, []).append(line)
	
	if len(categories) > 0:
		printtag=tag
		if tag[0] == 'v':
			printtag = tag[1:]
		print()
		print("#### Changes since", printtag + ":")
		print()
		for category in categories:
			print("#####", category)
			print()
			for line in categories[category]:
				print(line)
			print()
		if patchnotes:
			exit(0)
		print()

file = open(frozen, 'r')
print(file.read())
