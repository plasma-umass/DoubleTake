# -*- Makefile -*-

ACLOCAL_AMFLAGS  = -I m4
SUBDIRS          = doc src

pkgconfigdir     = @pkgconfigdir@
pkgconfig_DATA   = libHX.pc

${pkgconfig_DATA}: ${top_builddir}/config.status

tmpdir := $(shell mktemp -dtu)
packer  = xz
packext = .tar.xz

.PHONY: tarball
tarball:
# do not use mkdir_p here.
	mkdir ${tmpdir};
	pushd ${top_srcdir} && git archive --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD | tar -C ${tmpdir} -x && popd;
	pushd ${tmpdir}/${PACKAGE_NAME}-${PACKAGE_VERSION} && ./autogen.sh && pushd doc/ && ./generate && popd && popd;
	tar --use=${packer} -C ${tmpdir} -cf ${PACKAGE_NAME}-${PACKAGE_VERSION}${packext} --owner=root --group=root ${PACKAGE_NAME}-${PACKAGE_VERSION}/;
	rm -Rf ${tmpdir};
