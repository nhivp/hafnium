#
# Copyright (c) 2023, ARM Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    = -W
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = Hafnium
SOURCEDIR     = .
BUILDDIR      = build

V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif

# Put it first so that "make" without argument is like "make help".
help:
	${Q}$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	${Q}$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
