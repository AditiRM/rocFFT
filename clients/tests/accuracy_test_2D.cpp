// Copyright (c) 2016 - present Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <gtest/gtest.h>
#include <math.h>
#include <stdexcept>
#include <vector>

#include "../client_utils.h"

#include "accuracy_test.h"
#include "fftw_transform.h"
#include "rocfft.h"
#include "rocfft_against_fftw.h"

using ::testing::ValuesIn;

// Set parameters

// TODO: enable 16384, 32768 when omp support is available (takes too
// long!)
static std::vector<size_t> pow2_range = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

static std::vector<size_t> pow3_range = {3, 27, 81, 243, 729, 2187, 6561};

static std::vector<size_t> pow5_range = {5, 25, 125, 625, 3125, 15625};

static std::vector<size_t> prime_range = {7, 11, 13, 17, 19, 23, 29, 263, 269, 271, 277};

static std::vector<size_t> mix_range = {5000, 6000, 8000};

static std::vector<std::vector<size_t>> stride_range = {{1}};
static std::vector<std::vector<size_t>> mix_adhoc    = {{1, 22}, {1, 28}};

static std::vector<std::vector<size_t>> vpow2_range = {pow2_range, pow2_range};
INSTANTIATE_TEST_SUITE_P(
    pow2_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow2_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow2_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow2_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow2_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow2_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow2_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow2_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

static std::vector<std::vector<size_t>> vpow3_range = {pow3_range, pow3_range};
INSTANTIATE_TEST_SUITE_P(
    pow3_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow3_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow3_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow3_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow3_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow3_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow3_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow3_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

static std::vector<std::vector<size_t>> vpow5_range = {pow5_range, pow5_range};
INSTANTIATE_TEST_SUITE_P(
    pow5_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow5_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow5_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow5_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow5_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow5_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    pow5_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vpow5_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

static std::vector<std::vector<size_t>> vprime_range = {prime_range, prime_range};
INSTANTIATE_TEST_SUITE_P(
    prime_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vprime_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    prime_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vprime_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    prime_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vprime_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    prime_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vprime_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

static std::vector<std::vector<size_t>> vmix_range = {mix_range, mix_range};
INSTANTIATE_TEST_SUITE_P(
    mix_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vmix_range, mix_adhoc)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    mix_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vmix_range, mix_adhoc)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    mix_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vmix_range, mix_adhoc)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    mix_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vmix_range, mix_adhoc)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

// test length-1 on one dimension against a variety of non-1 lengths
static std::vector<std::vector<size_t>> vlen1_range = {{1}, {4, 8, 8192, 3, 27, 7, 11, 5000, 8000}};
INSTANTIATE_TEST_SUITE_P(
    len1_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);

// length-1 on the other dimension
static std::vector<std::vector<size_t>> vlen1_range_swap
    = {{4, 8, 8192, 3, 27, 7, 11, 5000, 8000}, {1}};
INSTANTIATE_TEST_SUITE_P(
    len1_swap_2D_complex_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range_swap)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_forward,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_swap_2D_complex_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range_swap)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_complex_inverse,
                                               place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_swap_2D_real_forward,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range_swap)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_forward, place_range))),
    accuracy_test::TestName);
INSTANTIATE_TEST_SUITE_P(
    len1_swap_2D_real_inverse,
    accuracy_test,
    ::testing::Combine(ValuesIn(generate_lengths(vlen1_range_swap)),
                       ValuesIn(precision_range),
                       ValuesIn(batch_range),
                       ValuesIn(stride_range),
                       ValuesIn(stride_range),
                       ValuesIn(generate_types(rocfft_transform_type_real_inverse, place_range))),
    accuracy_test::TestName);
