#! /bin/sh
#
# generate-ChangeLog.sh - generate ChangeLog from git log
#
# generate-ChangeLog.sh <top_srcdir> <distdir>
#
# Based in the instructions at
# http://erikhjortsberg.blogspot.com/2010/06/using-automake-to-generate-changelog.html
#

#set -x

builddir=$PWD
# top_srcdir is important for parallel-builds (i.e. bulding outside the
# source tree). It is relative to the directory where this script is run
# (hence we define ${builddir} above.
top_srcdir=$1

distdir=$2


# Check the ChangeLog length to avoid over-writing.
# Placeholder ChangeLog in the git repository has only a single line.
if [ `cat ${distdir}/ChangeLog | wc -l` = "1" ]; then
chmod u+w ${distdir}/ChangeLog && \
    cd ${top_srcdir} && \
    git log --stat --name-only --date=short --abbrev-commit > ${builddir}/${distdir}/ChangeLog
fi
