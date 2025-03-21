"""
Updates made by: Alexander Kedrowitsch <alexk1@vt.edu>

Aggregate changes for commits in range: 20247a4..275be8c

Modified/Added Function: build
  - Related commit message: slight change to account for the conflicting sdnv.h files in LTP and bundle-protocol

"""
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ltp-protocol', ['core','internet','network'])
    module.source = [
        'model/sdnv.cc',
        'model/ltp-protocol.cc',
        'model/ltp-header.cc',
        'model/ltp-queue-set.cc',
        'model/ltp-session-state-record.cc',
        'model/ltp-udp-convergence-layer-adapter.cc',
        'model/ltp-convergence-layer-adapter.cc',
        'model/ltp-ip-resolution-table.cc',
        'helper/ltp-protocol-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('ltp-protocol')
    module_test.source = [
        'test/ltp-protocol-test-suite.cc',
        'test/ltp-protocol-channel-loss-test-suite.cc'
        ]

    headers = bld(features='ns3header')
    headers.module = 'ltp-protocol'
    headers.source = [
        'model/ltp-protocol.h',
        'model/ltp-queue-set.h',
        'model/ltp-header.h',
        'model/ltp-session-state-record.h',
	'model/ltp-session-state-record-impl.h',
	'model/ltp-udp-convergence-layer-adapter.h',
	'model/ltp-convergence-layer-adapter.h',
	'model/ltp-ip-resolution-table.h',
        'helper/ltp-protocol-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

