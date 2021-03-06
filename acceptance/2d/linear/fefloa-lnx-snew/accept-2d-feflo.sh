#!/usr/bin/env bash

bin=${HOME}/refine/strict/two

${bin}/ref_acceptance 2 ref_fefloa_test.b8.ugrid

function adapt_cycle {
    proj=$1

    cp ref_fefloa_test.b8.ugrid ${proj}.b8.ugrid

    ${bin}/ref_translate ${proj}.b8.ugrid ${proj}.html
    ${bin}/ref_translate ${proj}.b8.ugrid ${proj}.tec

    ${bin}/ref_acceptance ${proj}.b8.ugrid ${proj}.metric 0.0001
    ${bin}/ref_fefloa2d ${proj}.b8.ugrid ${proj}.metric

    ~/codes/inria-file-tools/lnx-bin/feflo.a \
	-in fefloa.2d.meshb \
	-met fefloa.2d.sol \
        -snew \
	-out ref_fefloa_test.meshb 
    ${bin}/ref_translate ref_fefloa_test.meshb ref_fefloa_test.b8.ugrid
    
    ${bin}/ref_metric_test ${proj}.b8.ugrid ${proj}.metric | tee ${proj}.status
    cp ref_metric_test_s00_n1_p0_ellipse.tec ${proj}_metric_ellipse.tec
}

adapt_cycle accept-2d-fefloa-00
adapt_cycle accept-2d-fefloa-01
adapt_cycle accept-2d-fefloa-02
adapt_cycle accept-2d-fefloa-03
adapt_cycle accept-2d-fefloa-04
adapt_cycle accept-2d-fefloa-05
adapt_cycle accept-2d-fefloa-06
adapt_cycle accept-2d-fefloa-07
adapt_cycle accept-2d-fefloa-08
adapt_cycle accept-2d-fefloa-09

