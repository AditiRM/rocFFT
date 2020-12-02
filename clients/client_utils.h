// Copyright (c) 2020 - present Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <algorithm>
#include <complex>
#include <iostream>
#include <mutex>
#include <numeric>
#include <omp.h>
#include <random>
#include <tuple>
#include <vector>

#include "rocfft.h"
#include <hip/hip_runtime_api.h>

// Determine the size of the data type given the precision and type.
template <typename Tsize>
inline Tsize var_size(const rocfft_precision precision, const rocfft_array_type type)
{
    size_t var_size = 0;
    switch(precision)
    {
    case rocfft_precision_single:
        var_size = sizeof(float);
        break;
    case rocfft_precision_double:
        var_size = sizeof(double);
        break;
    }
    switch(type)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_hermitian_interleaved:
        var_size *= 2;
        break;
    default:
        break;
    }
    return var_size;
}

// Container class for test parameters.
class rocfft_params
{
public:
    // All parameters are row-major.
    std::vector<size_t>     length;
    std::vector<size_t>     istride;
    std::vector<size_t>     ostride;
    size_t                  nbatch         = 1;
    rocfft_precision        precision      = rocfft_precision_double;
    rocfft_transform_type   transform_type = rocfft_transform_type_complex_forward;
    rocfft_result_placement placement      = rocfft_placement_inplace;
    size_t                  idist          = 0;
    size_t                  odist          = 0;
    rocfft_array_type       itype          = rocfft_array_type_complex_interleaved;
    rocfft_array_type       otype          = rocfft_array_type_complex_interleaved;
    std::vector<size_t>     ioffset        = {0, 0};
    std::vector<size_t>     ooffset        = {0, 0};

    size_t isize = 0;
    size_t osize = 0;

    // Given an array type, return the name as a string.
    std::string array_type_name(const rocfft_array_type type) const
    {
        switch(type)
        {
        case rocfft_array_type_complex_interleaved:
            return "rocfft_array_type_complex_interleaved";
        case rocfft_array_type_complex_planar:
            return "rocfft_array_type_complex_planar";
        case rocfft_array_type_real:
            return "rocfft_array_type_real";
        case rocfft_array_type_hermitian_interleaved:
            return "rocfft_array_type_hermitian_interleaved";
        case rocfft_array_type_hermitian_planar:
            return "rocfft_array_type_hermitian_planar";
        case rocfft_array_type_unset:
            return "rocfft_array_type_unset";
        }
        return "";
    }

    // Convert to string for output.
    std::string str() const
    {
        std::stringstream ss;
        ss << "\nparams:\n";
        ss << "\tlength:";
        for(auto i : length)
            ss << " " << i;
        ss << "\n";
        ss << "\tistride:";
        for(auto i : istride)
            ss << " " << i;
        ss << "\n";
        ss << "\tidist: " << idist << "\n";

        ss << "\tostride:";
        for(auto i : ostride)
            ss << " " << i;
        ss << "\n";
        ss << "\todist: " << odist << "\n";

        ss << "\tbatch: " << nbatch << "\n";
        ss << "\tisize: " << isize << "\n";
        ss << "\tosize: " << osize << "\n";

        if(placement == rocfft_placement_inplace)
            ss << "\tin-place\n";
        else
            ss << "\tout-of-place\n";
        ss << "\t" << array_type_name(itype) << " -> " << array_type_name(otype) << "\n";
        if(precision == rocfft_precision_single)
            ss << "\tsingle-precision\n";
        else
            ss << "\tdouble-precision\n";

        ss << "\tilength:";
        for(auto i : ilength())
            ss << " " << i;
        ss << "\n";
        ss << "\tolength:";
        for(auto i : olength())
            ss << " " << i;
        ss << "\n";

        return ss.str();
    }

    // Dimension of the transform.
    size_t dim() const
    {
        return length.size();
    }
    std::vector<size_t> ilength()
    {
        auto ilength = length;
        if(transform_type == rocfft_transform_type_real_inverse)
            ilength[dim() - 1] = ilength[dim() - 1] / 2 + 1;
        return std::move(ilength);
    }
    std::vector<size_t> olength()
    {
        auto olength = length;
        if(transform_type == rocfft_transform_type_real_forward)
            olength[dim() - 1] = olength[dim() - 1] / 2 + 1;
        return std::move(olength);
    }

    std::vector<size_t> ibuffer_sizes() const
    {
        std::vector<size_t> ibuffer_sizes;
        if(osize == 0)
            return ibuffer_sizes;

        switch(itype)
        {
        case rocfft_array_type_complex_planar:
        case rocfft_array_type_hermitian_planar:
            ibuffer_sizes.resize(2);
            break;
        default:
            ibuffer_sizes.resize(1);
        }
        for(unsigned i = 0; i < ibuffer_sizes.size(); i++)
        {
            ibuffer_sizes[i] = isize * var_size<size_t>(precision, itype);
            ;
        }
        return ibuffer_sizes;
    }

    std::vector<size_t> obuffer_sizes() const
    {
        std::vector<size_t> obuffer_sizes;
        if(isize == 0)
            return obuffer_sizes;

        switch(otype)
        {
        case rocfft_array_type_complex_planar:
        case rocfft_array_type_hermitian_planar:
            obuffer_sizes.resize(2);
            break;
        default:
            obuffer_sizes.resize(1);
        }
        for(unsigned i = 0; i < obuffer_sizes.size(); i++)
        {
            obuffer_sizes[i] = osize * var_size<size_t>(precision, otype);
            ;
        }
        return obuffer_sizes;
    }

    // Estimate the amount of host memory needed.
    size_t needed_ram(const int verbose) const
    {
        // Host input, output, and input copy: 3 buffers, all contiguous.
        size_t needed_ram
            = 3 * std::accumulate(length.begin(), length.end(), 1, std::multiplies<size_t>());

        // GPU input buffer:
        needed_ram += std::inner_product(length.begin(), length.end(), istride.begin(), 0);

        // GPU output buffer:
        needed_ram += std::inner_product(length.begin(), length.end(), ostride.begin(), 0);

        // Account for precision and data type:
        if(transform_type != rocfft_transform_type_real_forward
           || transform_type != rocfft_transform_type_real_inverse)
        {
            needed_ram *= 2;
        }
        switch(precision)
        {
        case rocfft_precision_single:
            needed_ram *= 4;
            break;
        case rocfft_precision_double:
            needed_ram *= 8;
            break;
        }

        needed_ram *= nbatch;

        if(verbose > 1)
        {
            std::cout << "required host memory (GB): " << needed_ram * 1e-9 << std::endl;
        }

        return needed_ram;
    }

    // For real/complex transofrms, we need to account for differences in intput and output
    // dimensions.
    std::vector<size_t> ilength() const
    {
        auto ilength = length;
        if(transform_type == rocfft_transform_type_real_inverse)
            ilength[dim() - 1] = ilength[dim() - 1] / 2 + 1;
        return ilength;
    }
    std::vector<size_t> olength() const
    {
        auto olength = length;
        if(transform_type == rocfft_transform_type_real_forward)
            olength[dim() - 1] = olength[dim() - 1] / 2 + 1;
        return std::move(olength);
    }

    // Column-major getters:
    std::vector<size_t> ilength_cm() const
    {
        auto ilength_cm = ilength();
        std::reverse(std::begin(ilength_cm), std::end(ilength_cm));
        return std::move(ilength_cm);
    }
    std::vector<size_t> olength_cm() const
    {
        auto olength_cm = olength();
        std::reverse(std::begin(olength_cm), std::end(olength_cm));
        return std::move(olength_cm);
    }
    std::vector<size_t> length_cm() const
    {
        auto length_cm = length;
        std::reverse(std::begin(length_cm), std::end(length_cm));
        return std::move(length_cm);
    }

    std::vector<size_t> istride_cm() const
    {
        auto istride_cm = istride;
        std::reverse(std::begin(istride_cm), std::end(istride_cm));
        return std::move(istride_cm);
    }
    std::vector<size_t> ostride_cm() const
    {
        auto ostride_cm = ostride;
        std::reverse(std::begin(ostride_cm), std::end(ostride_cm));
        return std::move(ostride_cm);
    }

    // Number of input buffers
    int nibuffer() const
    {
        return (itype == rocfft_array_type_complex_planar
                || itype == rocfft_array_type_hermitian_planar)
                   ? 2
                   : 1;
    }

    // Number of output buffers
    int nobuffer() const
    {
        return (otype == rocfft_array_type_complex_planar
                || otype == rocfft_array_type_hermitian_planar)
                   ? 2
                   : 1;
    }

    // Return true if the given GPU parameters would produce a valid transform.
    bool valid(const int verbose) const
    {
        // Check that in-place transforms have the same input and output stride:
        if(placement == rocfft_placement_inplace)
        {
            const auto stridesize = std::min(istride.size(), ostride.size());
            bool       samestride = true;
            for(int i = 0; i < stridesize; ++i)
            {
                if(istride[i] != ostride[i])
                    samestride = false;
            }
            if(!samestride)
            {
                // In-place transforms require identical input and output strides.
                if(verbose)
                {
                    std::cout << "istride:";
                    for(const auto& i : istride)
                        std::cout << " " << i;
                    std::cout << " ostride0:";
                    for(const auto& i : ostride)
                        std::cout << " " << i;
                    std::cout << " differ; skipped for in-place transforms: skipping test"
                              << std::endl;
                }
                // TODO: mark skipped
                return false;
            }

            if((transform_type == rocfft_transform_type_real_forward
                || transform_type == rocfft_transform_type_real_inverse)
               && (istride[0] != 1 || ostride[0] != 1))
            {
                // In-place real/complex transforms require unit strides.
                if(verbose)
                {
                    std::cout
                        << "istride[0]: " << istride[0] << " ostride[0]: " << ostride[0]
                        << " must be unitary for in-place real/complex transforms: skipping test"
                        << std::endl;
                }
                return false;
            }

            if((itype == rocfft_array_type_complex_interleaved
                && otype == rocfft_array_type_complex_planar)
               || (itype == rocfft_array_type_complex_planar
                   && otype == rocfft_array_type_complex_interleaved))
            {
                if(verbose)
                {
                    std::cout << "In-place c2c transforms require identical io types; skipped.\n";
                }
                return false;
            }
        }

        // The parameters are valid.
        return true;
    }
};

// This is used with the program_options class so that the user can type an integer on the
// command line and we store into an enum varaible
template <typename _Elem, typename _Traits>
std::basic_istream<_Elem, _Traits>& operator>>(std::basic_istream<_Elem, _Traits>& stream,
                                               rocfft_array_type&                  atype)
{
    unsigned tmp;
    stream >> tmp;
    atype = rocfft_array_type(tmp);
    return stream;
}

// similarly for transform type
template <typename _Elem, typename _Traits>
std::basic_istream<_Elem, _Traits>& operator>>(std::basic_istream<_Elem, _Traits>& stream,
                                               rocfft_transform_type&              ttype)
{
    unsigned tmp;
    stream >> tmp;
    ttype = rocfft_transform_type(tmp);
    return stream;
}

template <typename T1, typename T2>
static bool increment_base(T1& index, const T2& length)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    if(index < length - 1)
    {
        ++index;
        return true;
    }
    index = 0;
    return false;
}

// Increment the index (column-major) for looping over 1, 2, and 3 dimensions length.
template <typename T1, typename T2>
static bool increment_colmajor(T1& index, const T2& length)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    return increment_base(index, length);
}

template <typename T1, typename T2>
static bool increment_colmajor(std::tuple<T1, T1>& index, const std::tuple<T2, T2>& length)
{
    if(increment_base(std::get<0>(index), std::get<0>(length)))
        // we incremented ok, nothing further to do
        return true;
    // otherwise, we rolled over
    return increment_base(std::get<1>(index), std::get<1>(length));
}

template <typename T1, typename T2>
static bool increment_colmajor(std::tuple<T1, T1, T1>& index, const std::tuple<T2, T2, T2>& length)
{
    if(increment_base(std::get<0>(index), std::get<0>(length)))
        // we incremented ok, nothing further to do
        return true;
    if(increment_base(std::get<1>(index), std::get<1>(length)))
        // we incremented ok, nothing further to do
        return true;
    // otherwise, we rolled over
    return increment_base(std::get<2>(index), std::get<2>(length));
}

// Increment column-major index over arbitrary dimension length
template <typename T1, typename T2>
inline bool increment_colmajor(std::vector<T1>& index, const std::vector<T2>& length)
{
    for(int idim = 0; idim < length.size(); ++idim)
    {
        if(index[idim] < length[idim])
        {
            if(++index[idim] == length[idim])
            {
                index[idim] = 0;
                continue;
            }
            // we know we were able to increment something and didn't hit the end
            return true;
        }
    }
    // End the loop when we get back to the start:
    return !std::all_of(index.begin(), index.end(), [](int i) { return i == 0; });
}

// Increment the index (row-major) for looping over 1, 2, and 3 dimensions length.
template <typename T1, typename T2>
static bool increment_rowmajor(T1& index, const T2& length)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    return increment_base(index, length);
}

template <typename T1, typename T2>
static bool increment_rowmajor(std::tuple<T1, T1>& index, const std::tuple<T2, T2>& length)
{
    if(increment_base(std::get<1>(index), std::get<1>(length)))
        // we incremented ok, nothing further to do
        return true;
    // otherwise, we rolled over
    return increment_base(std::get<0>(index), std::get<0>(length));
}

template <typename T1, typename T2>
static bool increment_rowmajor(std::tuple<T1, T1, T1>& index, const std::tuple<T2, T2, T2>& length)
{
    if(increment_base(std::get<2>(index), std::get<2>(length)))
        // we incremented ok, nothing further to do
        return true;
    if(increment_base(std::get<1>(index), std::get<1>(length)))
        // we incremented ok, nothing further to do
        return true;
    // otherwise, we rolled over
    return increment_base(std::get<0>(index), std::get<0>(length));
}

// Increment row-major index over arbitrary dimension length
template <typename T1, typename T2>
bool increment_rowmajor(std::vector<T1>& index, const std::vector<T2>& length)
{
    for(int idim = length.size(); idim-- > 0;)
    {
        if(index[idim] < length[idim])
        {
            if(++index[idim] == length[idim])
            {
                index[idim] = 0;
                continue;
            }
            // we know we were able to increment something and didn't hit the end
            return true;
        }
    }
    // End the loop when we get back to the start:
    return !std::all_of(index.begin(), index.end(), [](int i) { return i == 0; });
}

// count the number of total iterations for 1-, 2-, and 3-D dimensions
template <typename T1>
size_t count_iters(const T1& i)
{
    return i;
}

template <typename T1>
size_t count_iters(const std::tuple<T1, T1>& i)
{
    return std::get<0>(i) * std::get<1>(i);
}

template <typename T1>
size_t count_iters(const std::tuple<T1, T1, T1>& i)
{
    return std::get<0>(i) * std::get<1>(i) * std::get<2>(i);
}

// work out how many partitions to break our iteration problem into
template <typename T1>
static size_t compute_partition_count(T1 length)
{
#ifdef BUILD_CLIENTS_TESTS_OPENMP
    // we seem to get contention from too many threads, which slows
    // things down.  particularly noticeable with mix_3D tests
    static const size_t MAX_PARTITIONS = 8;
    size_t              iters          = count_iters(length);
    size_t hw_threads = std::min(MAX_PARTITIONS, static_cast<size_t>(omp_get_num_procs()));
    if(!hw_threads)
        return 1;

    // don't bother threading problem sizes that are too small. pick
    // an arbitrary number of iterations and ensure that each thread
    // has at least that many iterations to process
    static const size_t MIN_ITERS_PER_THREAD = 2048;

    // either use the whole CPU, or use ceil(iters/iters_per_thread)
    return std::min(hw_threads, (iters + MIN_ITERS_PER_THREAD + 1) / MIN_ITERS_PER_THREAD);
#else
    return 1;
#endif
}

// break a scalar length into some number of pieces, returning
// [(start0, end0), (start1, end1), ...]
template <typename T1>
std::vector<std::pair<T1, T1>> partition_base(const T1& length, size_t num_parts)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");

    // make sure we don't exceed the length
    num_parts = std::min(length, num_parts);

    std::vector<std::pair<T1, T1>> ret(num_parts);
    auto                           partition_size = length / num_parts;
    T1                             cur_partition  = 0;
    for(size_t i = 0; i < num_parts; ++i, cur_partition += partition_size)
    {
        ret[i].first  = cur_partition;
        ret[i].second = cur_partition + partition_size;
    }
    // last partition might not divide evenly, fix it up
    ret.back().second = length;
    return ret;
}

// returns pairs of startindex, endindex, for 1D, 2D, 3D lengths
template <typename T1>
std::vector<std::pair<T1, T1>> partition_rowmajor(const T1& length)
{
    return partition_base(length, compute_partition_count(length));
}

// partition on the leftmost part of the tuple, for row-major indexing
template <typename T1>
std::vector<std::pair<std::tuple<T1, T1>, std::tuple<T1, T1>>>
    partition_rowmajor(const std::tuple<T1, T1>& length)
{
    auto partitions = partition_base(std::get<0>(length), compute_partition_count(length));
    std::vector<std::pair<std::tuple<T1, T1>, std::tuple<T1, T1>>> ret(partitions.size());
    for(size_t i = 0; i < partitions.size(); ++i)
    {
        std::get<0>(ret[i].first)  = partitions[i].first;
        std::get<1>(ret[i].first)  = 0;
        std::get<0>(ret[i].second) = partitions[i].second;
        std::get<1>(ret[i].second) = std::get<1>(length);
    }
    return ret;
}
template <typename T1>
std::vector<std::pair<std::tuple<T1, T1, T1>, std::tuple<T1, T1, T1>>>
    partition_rowmajor(const std::tuple<T1, T1, T1>& length)
{
    auto partitions = partition_base(std::get<0>(length), compute_partition_count(length));
    std::vector<std::pair<std::tuple<T1, T1, T1>, std::tuple<T1, T1, T1>>> ret(partitions.size());
    for(size_t i = 0; i < partitions.size(); ++i)
    {
        std::get<0>(ret[i].first)  = partitions[i].first;
        std::get<1>(ret[i].first)  = 0;
        std::get<2>(ret[i].first)  = 0;
        std::get<0>(ret[i].second) = partitions[i].second;
        std::get<1>(ret[i].second) = std::get<1>(length);
        std::get<2>(ret[i].second) = std::get<2>(length);
    }
    return ret;
}

// returns pairs of startindex, endindex, for 1D, 2D, 3D lengths
template <typename T1>
std::vector<std::pair<T1, T1>> partition_colmajor(const T1& length)
{
    return partition_base(length, compute_partition_count(length));
}

// partition on the rightmost part of the tuple, for col-major indexing
template <typename T1>
std::vector<std::pair<std::tuple<T1, T1>, std::tuple<T1, T1>>>
    partition_colmajor(const std::tuple<T1, T1>& length)
{
    auto partitions = partition_base(std::get<1>(length), compute_partition_count(length));
    std::vector<std::pair<std::tuple<T1, T1>, std::tuple<T1, T1>>> ret(partitions.size());
    for(size_t i = 0; i < partitions.size(); ++i)
    {
        std::get<1>(ret[i].first)  = partitions[i].first;
        std::get<0>(ret[i].first)  = 0;
        std::get<1>(ret[i].second) = partitions[i].second;
        std::get<0>(ret[i].second) = std::get<0>(length);
    }
    return ret;
}
template <typename T1>
std::vector<std::pair<std::tuple<T1, T1, T1>, std::tuple<T1, T1, T1>>>
    partition_colmajor(const std::tuple<T1, T1, T1>& length)
{
    auto partitions = partition_base(std::get<2>(length), compute_partition_count(length));
    std::vector<std::pair<std::tuple<T1, T1, T1>, std::tuple<T1, T1, T1>>> ret(partitions.size());
    for(size_t i = 0; i < partitions.size(); ++i)
    {
        std::get<2>(ret[i].first)  = partitions[i].first;
        std::get<1>(ret[i].first)  = 0;
        std::get<0>(ret[i].first)  = 0;
        std::get<2>(ret[i].second) = partitions[i].second;
        std::get<1>(ret[i].second) = std::get<1>(length);
        std::get<0>(ret[i].second) = std::get<0>(length);
    }
    return ret;
}

// specialized computation of index given 1-, 2-, 3- dimension length + stride
template <typename T1, typename T2>
int compute_index(T1 length, T2 stride, size_t base)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    return (length * stride) + base;
}

template <typename T1, typename T2>
int compute_index(const std::tuple<T1, T1>& length, const std::tuple<T2, T2>& stride, size_t base)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    return (std::get<0>(length) * std::get<0>(stride)) + (std::get<1>(length) * std::get<1>(stride))
           + base;
}

template <typename T1, typename T2>
int compute_index(const std::tuple<T1, T1, T1>& length,
                  const std::tuple<T2, T2, T2>& stride,
                  size_t                        base)
{
    static_assert(std::is_integral<T1>::value, "Integral required.");
    static_assert(std::is_integral<T2>::value, "Integral required.");
    return (std::get<0>(length) * std::get<0>(stride)) + (std::get<1>(length) * std::get<1>(stride))
           + (std::get<2>(length) * std::get<2>(stride)) + base;
}

// Output a formatted general-dimensional array with given length and stride in batches
// separated by dist.
template <typename Toutput, typename T1, typename T2, typename Tsize>
inline void printbuffer(const Toutput*         output,
                        const std::vector<T1>& length,
                        const std::vector<T2>& stride,
                        const Tsize            nbatch,
                        const Tsize            dist)
{
    auto i_base = 0;
    for(auto b = 0; b < nbatch; b++, i_base += dist)
    {
        std::vector<int> index(length.size());
        std::fill(index.begin(), index.end(), 0);
        do
        {
            const int i = std::inner_product(index.begin(), index.end(), stride.begin(), i_base);
            std::cout << output[i] << " ";
            for(int i = index.size(); i-- > 0;)
            {
                if(index[i] == (length[i] - 1))
                {
                    std::cout << "\n";
                }
                else
                {
                    break;
                }
            }
        } while(increment_rowmajor(index, length));
        std::cout << std::endl;
    }
}

// Print a buffer stored as a std::vector of chars.
// Template types Tint1 and Tint2 are integer types
template <typename Tint1, typename Tint2, typename Tallocator>
inline void printbuffer(const rocfft_precision                            precision,
                        const rocfft_array_type                           itype,
                        const std::vector<std::vector<char, Tallocator>>& buf,
                        const std::vector<Tint1>&                         length,
                        const std::vector<Tint2>&                         stride,
                        const size_t                                      nbatch,
                        const size_t                                      dist)
{
    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_hermitian_interleaved:
        if(precision == rocfft_precision_double)
        {
            printbuffer((std::complex<double>*)buf[0].data(), length, stride, nbatch, dist);
        }
        else
        {
            printbuffer((std::complex<float>*)buf[0].data(), length, stride, nbatch, dist);
        }
        break;
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_planar:
        if(precision == rocfft_precision_double)
        {
            printbuffer((double*)buf[0].data(), length, stride, nbatch, dist);
            printbuffer((double*)buf[1].data(), length, stride, nbatch, dist);
        }
        else
        {
            printbuffer((float*)buf[0].data(), length, stride, nbatch, dist);
            printbuffer((float*)buf[1].data(), length, stride, nbatch, dist);
        }
        break;
    case rocfft_array_type_real:
        if(precision == rocfft_precision_double)
        {
            printbuffer((double*)buf[0].data(), length, stride, nbatch, dist);
        }
        else
        {
            printbuffer((float*)buf[0].data(), length, stride, nbatch, dist);
        }
        break;
    default:
        std::cout << "unkown array type\n";
    }
}

// Print the contents of a buffer stored as a std::vector of chars.  The output is flat,
// ie the entire memory range is printed as though it were a contiguous 1D array.
template <typename Tallocator>
inline void printbuffer_flat(const rocfft_precision                            precision,
                             const rocfft_array_type                           itype,
                             const std::vector<std::vector<char, Tallocator>>& buf,
                             const size_t                                      dist)
{
    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_hermitian_interleaved:
        if(precision == rocfft_precision_double)
        {
            auto data = reinterpret_cast<const std::complex<double>*>(buf[0].data());
            std::cout << "idx " << 0;
            for(size_t i = 0; i < dist; ++i)
                std::cout << " " << data[i];
            std::cout << std::endl;
        }
        else
        {
            auto data = reinterpret_cast<const std::complex<float>*>(buf[0].data());
            std::cout << "idx " << 0;
            for(size_t i = 0; i < dist; ++i)
                std::cout << " " << data[i];
            std::cout << std::endl;
        }
        break;
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_planar:
        if(precision == rocfft_precision_double)
        {
            for(int idx = 0; idx < buf.size(); ++idx)
            {
                auto data = reinterpret_cast<const double*>(buf[idx].data());
                std::cout << "idx " << idx;
                for(size_t i = 0; i < dist; ++i)
                    std::cout << " " << data[i];
                std::cout << std::endl;
            }
        }
        else
        {
            for(int idx = 0; idx < buf.size(); ++idx)
            {
                auto data = reinterpret_cast<const float*>(buf[idx].data());
                std::cout << "idx " << idx;
                for(size_t i = 0; i < dist; ++i)
                    std::cout << " " << data[i];
                std::cout << std::endl;
            }
        }
        break;
    case rocfft_array_type_real:
        if(precision == rocfft_precision_double)
        {
            auto data = reinterpret_cast<const double*>(buf[0].data());
            std::cout << "idx " << 0;
            for(size_t i = 0; i < dist; ++i)
                std::cout << " " << data[i];
            std::cout << std::endl;
        }
        else
        {
            auto data = reinterpret_cast<const float*>(buf[0].data());
            std::cout << "idx " << 0;
            for(size_t i = 0; i < dist; ++i)
                std::cout << " " << data[i];
            std::cout << std::endl;
        }
        break;
    default:
        std::cout << "unkown array type\n";
    }
}

// Given a length vector, set the rest of the strides.
// The optional argument stride0 sets the stride for the contiguous dimension.
// The optional rcpadding argument sets the stride correctly for in-place
// multi-dimensional real/complex transforms.
// Format is row-major.
template <typename T1>
inline std::vector<T1> compute_stride(const std::vector<T1>&     length,
                                      const std::vector<size_t>& stride0   = std::vector<size_t>(),
                                      const bool                 rcpadding = false)
{
    // We can't have more strides than dimensions:
    assert(stride0.size() <= length.size());

    const int dim = length.size();

    std::vector<T1> stride(dim);

    int dimoffset = 0;

    if(stride0.size() == 0)
    {
        // Set the contiguous stride:
        stride[dim - 1] = 1;
        dimoffset       = 1;
    }
    else
    {
        // Copy the input values to the end of the stride array:
        for(int i = 0; i < stride0.size(); ++i)
        {
            stride[dim - stride0.size() + i] = stride0[i];
        }
    }

    if(stride0.size() < dim)
    {
        // Compute any remaining values via recursion.
        for(int i = dim - dimoffset - stride0.size(); i-- > 0;)
        {
            auto lengthip1 = length[i + 1];
            if(rcpadding && i == dim - 2)
            {
                lengthip1 = 2 * (lengthip1 / 2 + 1);
            }
            stride[i] = stride[i + 1] * lengthip1;
        }
    }

    return stride;
}

// Copy data of dimensions length with strides istride and length idist between batches to
// a buffer with strides ostride and length odist between batches.  The input and output
// types are identical.
template <typename Tval, typename Tint1, typename Tint2, typename Tint3>
inline void copy_buffers_1to1(const Tval*  input,
                              Tval*        output,
                              const Tint1& whole_length,
                              const size_t nbatch,
                              const Tint2& istride,
                              const size_t idist,
                              const Tint3& ostride,
                              const size_t odist)
{
    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            auto       index  = partitions[part].first;
            const auto length = partitions[part].second;
            do
            {
                const int idx = compute_index(index, istride, idx_base);
                const int odx = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                output[odx]   = input[idx];
            } while(increment_rowmajor(index, length));
        }
    }
}

// Copy data of dimensions length with strides istride and length idist between batches to
// a buffer with strides ostride and length odist between batches.  The input type is
// planar and the output type is complex interleaved.
template <typename Tval, typename Tint1, typename Tint2, typename Tint3>
inline void copy_buffers_2to1(const Tval*         input0,
                              const Tval*         input1,
                              std::complex<Tval>* output,
                              const Tint1&        whole_length,
                              const size_t        nbatch,
                              const Tint2&        istride,
                              const size_t        idist,
                              const Tint3&        ostride,
                              const size_t        odist)
{
    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            auto       index  = partitions[part].first;
            const auto length = partitions[part].second;
            do
            {
                const int idx = compute_index(index, istride, idx_base);
                const int odx = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                output[odx]   = std::complex<Tval>(input0[idx], input1[idx]);
            } while(increment_rowmajor(index, length));
        }
    }
}

// Copy data of dimensions length with strides istride and length idist between batches to
// a buffer with strides ostride and length odist between batches.  The input type is
// complex interleaved and the output type is planar.
template <typename Tval, typename Tint1, typename Tint2, typename Tint3>
inline void copy_buffers_1to2(const std::complex<Tval>* input,
                              Tval*                     output0,
                              Tval*                     output1,
                              const Tint1&              whole_length,
                              const size_t              nbatch,
                              const Tint2&              istride,
                              const size_t              idist,
                              const Tint3&              ostride,
                              const size_t              odist)
{
    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            auto       index  = partitions[part].first;
            const auto length = partitions[part].second;
            do
            {
                const int idx = compute_index(index, istride, idx_base);
                const int odx = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                output0[odx]  = input[idx].real();
                output1[odx]  = input[idx].imag();
            } while(increment_rowmajor(index, length));
        }
    }
}

// Copy data of dimensions length with strides istride and length idist between batches to
// a buffer with strides ostride and length odist between batches.  The input type given
// by itype, and the output type is given by otype.
template <typename Tallocator1,
          typename Tallocator2,
          typename Tint1,
          typename Tint2,
          typename Tint3>
inline void copy_buffers(const std::vector<std::vector<char, Tallocator1>>& input,
                         std::vector<std::vector<char, Tallocator2>>&       output,
                         const Tint1&                                       length,
                         const size_t                                       nbatch,
                         const rocfft_precision                             precision,
                         const rocfft_array_type                            itype,
                         const Tint2&                                       istride,
                         const size_t                                       idist,
                         const rocfft_array_type                            otype,
                         const Tint3&                                       ostride,
                         const size_t                                       odist)
{
    if(itype == otype)
    {
        switch(itype)
        {
        case rocfft_array_type_complex_interleaved:
        case rocfft_array_type_hermitian_interleaved:
            switch(precision)
            {
            case rocfft_precision_single:
                copy_buffers_1to1(reinterpret_cast<const std::complex<float>*>(input[0].data()),
                                  reinterpret_cast<std::complex<float>*>(output[0].data()),
                                  length,
                                  nbatch,
                                  istride,
                                  idist,
                                  ostride,
                                  odist);
                break;
            case rocfft_precision_double:
                copy_buffers_1to1(reinterpret_cast<const std::complex<double>*>(input[0].data()),
                                  reinterpret_cast<std::complex<double>*>(output[0].data()),
                                  length,
                                  nbatch,
                                  istride,
                                  idist,
                                  ostride,
                                  odist);
                break;
            }
            break;
        case rocfft_array_type_real:
        case rocfft_array_type_complex_planar:
        case rocfft_array_type_hermitian_planar:
            for(int idx = 0; idx < input.size(); ++idx)
            {
                switch(precision)
                {
                case rocfft_precision_single:
                    copy_buffers_1to1(reinterpret_cast<const float*>(input[idx].data()),
                                      reinterpret_cast<float*>(output[idx].data()),
                                      length,
                                      nbatch,
                                      istride,
                                      idist,
                                      ostride,
                                      odist);
                    break;
                case rocfft_precision_double:
                    copy_buffers_1to1(reinterpret_cast<const double*>(input[idx].data()),
                                      reinterpret_cast<double*>(output[idx].data()),
                                      length,
                                      nbatch,
                                      istride,
                                      idist,
                                      ostride,
                                      odist);
                    break;
                }
            }
            break;
        default:
            throw std::runtime_error("Invalid data type");
            break;
        }
    }
    else if((itype == rocfft_array_type_complex_interleaved
             && otype == rocfft_array_type_complex_planar)
            || (itype == rocfft_array_type_hermitian_interleaved
                && otype == rocfft_array_type_hermitian_planar))
    {
        // copy 1to2
        switch(precision)
        {
        case rocfft_precision_single:
            copy_buffers_1to2(reinterpret_cast<const std::complex<float>*>(input[0].data()),
                              reinterpret_cast<float*>(output[0].data()),
                              reinterpret_cast<float*>(output[1].data()),
                              length,
                              nbatch,
                              istride,
                              idist,
                              ostride,
                              odist);
            break;
        case rocfft_precision_double:
            copy_buffers_1to2(reinterpret_cast<const std::complex<double>*>(input[0].data()),
                              reinterpret_cast<double*>(output[0].data()),
                              reinterpret_cast<double*>(output[1].data()),
                              length,
                              nbatch,
                              istride,
                              idist,
                              ostride,
                              odist);
            break;
        }
    }
    else if((itype == rocfft_array_type_complex_planar
             && otype == rocfft_array_type_complex_interleaved)
            || (itype == rocfft_array_type_hermitian_planar
                && otype == rocfft_array_type_hermitian_interleaved))
    {
        // copy 2 to 1
        switch(precision)
        {
        case rocfft_precision_single:
            copy_buffers_2to1(reinterpret_cast<const float*>(input[0].data()),
                              reinterpret_cast<const float*>(input[1].data()),
                              reinterpret_cast<std::complex<float>*>(output[0].data()),
                              length,
                              nbatch,
                              istride,
                              idist,
                              ostride,
                              odist);
            break;
        case rocfft_precision_double:
            copy_buffers_2to1(reinterpret_cast<const double*>(input[0].data()),
                              reinterpret_cast<const double*>(input[1].data()),
                              reinterpret_cast<std::complex<double>*>(output[0].data()),
                              length,
                              nbatch,
                              istride,
                              idist,
                              ostride,
                              odist);
            break;
        }
    }
    else
    {
        throw std::runtime_error("Invalid input and output types.");
    }
}

// unroll arbitrary-dimension copy_buffers into specializations for 1-, 2-, 3-dimensions
template <typename Tallocator1,
          typename Tallocator2,
          typename Tint1,
          typename Tint2,
          typename Tint3>
inline void copy_buffers(const std::vector<std::vector<char, Tallocator1>>& input,
                         std::vector<std::vector<char, Tallocator2>>&       output,
                         const std::vector<Tint1>&                          length,
                         const size_t                                       nbatch,
                         const rocfft_precision                             precision,
                         const rocfft_array_type                            itype,
                         const std::vector<Tint2>&                          istride,
                         const size_t                                       idist,
                         const rocfft_array_type                            otype,
                         const std::vector<Tint3>&                          ostride,
                         const size_t                                       odist)
{
    switch(length.size())
    {
    case 1:
        return copy_buffers(input,
                            output,
                            length[0],
                            nbatch,
                            precision,
                            itype,
                            istride[0],
                            idist,
                            otype,
                            ostride[0],
                            odist);
    case 2:
        return copy_buffers(input,
                            output,
                            std::make_tuple(length[0], length[1]),
                            nbatch,
                            precision,
                            itype,
                            std::make_tuple(istride[0], istride[1]),
                            idist,
                            otype,
                            std::make_tuple(ostride[0], ostride[1]),
                            odist);
    case 3:
        return copy_buffers(input,
                            output,
                            std::make_tuple(length[0], length[1], length[2]),
                            nbatch,
                            precision,
                            itype,
                            std::make_tuple(istride[0], istride[1], istride[2]),
                            idist,
                            otype,
                            std::make_tuple(ostride[0], ostride[1], ostride[2]),
                            odist);
    default:
        abort();
    }
}

// Compute the L-infinity and L-2 distance between two buffers with strides istride and
// length idist between batches to a buffer with strides ostride and length odist between
// batches.  Both buffers are of complex type.

struct VectorNorms
{
    double l_2 = 0.0, l_inf = 0.0;
};

template <typename Tcomplex, typename Tint1, typename Tint2, typename Tint3>
inline VectorNorms distance_1to1_complex(const Tcomplex*                         input,
                                         const Tcomplex*                         output,
                                         const Tint1&                            whole_length,
                                         const size_t                            nbatch,
                                         const Tint2&                            istride,
                                         const size_t                            idist,
                                         const Tint3&                            ostride,
                                         const size_t                            odist,
                                         std::vector<std::pair<size_t, size_t>>& linf_failures,
                                         const double                            linf_cutoff)
{
    double linf = 0.0;
    double l2   = 0.0;

    std::mutex linf_failure_lock;

    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_colmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for reduction(max : linf) reduction(+ : l2) num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            double     cur_linf = 0.0;
            double     cur_l2   = 0.0;
            auto       index    = partitions[part].first;
            const auto length   = partitions[part].second;

            do
            {
                const int    idx   = compute_index(index, istride, idx_base);
                const int    odx   = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                const double rdiff = std::abs(output[odx].real() - input[idx].real());
                cur_linf           = std::max(rdiff, cur_linf);
                if(cur_linf > linf_cutoff)
                {
                    std::pair<size_t, size_t> fval(b, idx);
                    linf_failure_lock.lock();
                    linf_failures.push_back(fval);
                    linf_failure_lock.unlock();
                }
                cur_l2 += rdiff * rdiff;

                const double idiff = std::abs(output[odx].imag() - input[idx].imag());
                cur_linf           = std::max(idiff, cur_linf);
                if(cur_linf > linf_cutoff)
                {
                    std::pair<size_t, size_t> fval(b, idx);
                    linf_failure_lock.lock();
                    linf_failures.push_back(fval);
                    linf_failure_lock.unlock();
                }
                cur_l2 += idiff * idiff;

            } while(increment_colmajor(index, length));
            linf = std::max(linf, cur_linf);
            l2 += cur_l2;
        }
    }
    return {.l_2 = sqrt(l2), .l_inf = linf};
}

// Compute the L-infinity and L-2 distance between two buffers with strides istride and
// length idist between batches to a buffer with strides ostride and length odist between
// batches.  Both buffers are of real type.
template <typename Tfloat, typename Tint1, typename Tint2, typename Tint3>
inline VectorNorms distance_1to1_real(const Tfloat*                           input,
                                      const Tfloat*                           output,
                                      const Tint1&                            whole_length,
                                      const size_t                            nbatch,
                                      const Tint2&                            istride,
                                      const size_t                            idist,
                                      const Tint3&                            ostride,
                                      const size_t                            odist,
                                      std::vector<std::pair<size_t, size_t>>& linf_failures,
                                      const double                            linf_cutoff)
{
    double linf = 0.0;
    double l2   = 0.0;

    std::mutex linf_failure_lock;

    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for reduction(max : linf) reduction(+ : l2) num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            double     cur_linf = 0.0;
            double     cur_l2   = 0.0;
            auto       index    = partitions[part].first;
            const auto length   = partitions[part].second;
            do
            {
                const int    idx  = compute_index(index, istride, idx_base);
                const int    odx  = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                const double diff = std::abs(output[odx] - input[idx]);
                cur_linf          = std::max(diff, cur_linf);
                if(cur_linf > linf_cutoff)
                {
                    std::pair<size_t, size_t> fval(b, idx);
                    linf_failure_lock.lock();
                    linf_failures.push_back(fval);
                    linf_failure_lock.unlock();
                }
                cur_l2 += diff * diff;

            } while(increment_rowmajor(index, length));
            linf = std::max(linf, cur_linf);
            l2 += cur_l2;
        }
    }
    return {.l_2 = sqrt(l2), .l_inf = linf};
}

// Compute the L-infinity and L-2 distance between two buffers with strides istride and
// length idist between batches to a buffer with strides ostride and length odist between
// batches.  input is complex-interleaved, output is complex-planar.
template <typename Tval, typename Tint1, typename T2, typename T3>
inline VectorNorms distance_1to2(const std::complex<Tval>*               input,
                                 const Tval*                             output0,
                                 const Tval*                             output1,
                                 const Tint1&                            whole_length,
                                 const size_t                            nbatch,
                                 const T2&                               istride,
                                 const size_t                            idist,
                                 const T3&                               ostride,
                                 const size_t                            odist,
                                 std::vector<std::pair<size_t, size_t>>& linf_failures,
                                 const double                            linf_cutoff)
{
    double linf = 0.0;
    double l2   = 0.0;

    std::mutex linf_failure_lock;

    bool   idx_equals_odx = istride == ostride && idist == odist;
    size_t idx_base       = 0;
    size_t odx_base       = 0;
    auto   partitions     = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist, odx_base += odist)
    {
#pragma omp parallel for reduction(max : linf) reduction(+ : l2) num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            double     cur_linf = 0.0;
            double     cur_l2   = 0.0;
            auto       index    = partitions[part].first;
            const auto length   = partitions[part].second;
            do
            {
                const int    idx   = compute_index(index, istride, idx_base);
                const int    odx   = idx_equals_odx ? idx : compute_index(index, ostride, odx_base);
                const double rdiff = std::abs(output0[odx] - input[idx].real());
                cur_linf           = std::max(rdiff, cur_linf);
                if(cur_linf > linf_cutoff)
                {
                    std::pair<size_t, size_t> fval(b, idx);
                    linf_failure_lock.lock();
                    linf_failures.push_back(fval);
                    linf_failure_lock.unlock();
                }
                cur_l2 += rdiff * rdiff;

                const double idiff = std::abs(output1[odx] - input[idx].imag());
                cur_linf           = std::max(idiff, cur_linf);
                if(cur_linf > linf_cutoff)
                {
                    std::pair<size_t, size_t> fval(b, idx);
                    linf_failure_lock.lock();
                    linf_failures.push_back(fval);
                    linf_failure_lock.unlock();
                }
                cur_l2 += idiff * idiff;

            } while(increment_rowmajor(index, length));
            linf = std::max(linf, cur_linf);
            l2 += cur_l2;
        }
    }
    return {.l_2 = sqrt(l2), .l_inf = linf};
}

// Compute the L-inifnity and L-2 distance between two buffers of dimension length and
// with types given by itype, otype, and precision.
template <typename Tallocator1,
          typename Tallocator2,
          typename Tint1,
          typename Tint2,
          typename Tint3>
inline VectorNorms distance(const std::vector<std::vector<char, Tallocator1>>& input,
                            const std::vector<std::vector<char, Tallocator2>>& output,
                            const Tint1&                                       length,
                            const size_t                                       nbatch,
                            const rocfft_precision                             precision,
                            const rocfft_array_type                            itype,
                            const Tint2&                                       istride,
                            const size_t                                       idist,
                            const rocfft_array_type                            otype,
                            const Tint3&                                       ostride,
                            const size_t                                       odist,
                            std::vector<std::pair<size_t, size_t>>&            linf_failures,
                            const double                                       linf_cutoff)
{
    VectorNorms dist;

    if(itype == otype)
    {
        switch(itype)
        {
        case rocfft_array_type_complex_interleaved:
        case rocfft_array_type_hermitian_interleaved:
            switch(precision)
            {
            case rocfft_precision_single:
                dist = distance_1to1_complex(
                    reinterpret_cast<const std::complex<float>*>(input[0].data()),
                    reinterpret_cast<const std::complex<float>*>(output[0].data()),
                    length,
                    nbatch,
                    istride,
                    idist,
                    ostride,
                    odist,
                    linf_failures,
                    linf_cutoff);
                break;
            case rocfft_precision_double:
                dist = distance_1to1_complex(
                    reinterpret_cast<const std::complex<double>*>(input[0].data()),
                    reinterpret_cast<const std::complex<double>*>(output[0].data()),
                    length,
                    nbatch,
                    istride,
                    idist,
                    ostride,
                    odist,
                    linf_failures,
                    linf_cutoff);
                break;
            }
            dist.l_2 *= dist.l_2;
            break;
        case rocfft_array_type_real:
        case rocfft_array_type_complex_planar:
        case rocfft_array_type_hermitian_planar:
            for(int idx = 0; idx < input.size(); ++idx)
            {
                VectorNorms d;
                switch(precision)
                {
                case rocfft_precision_single:
                    d = distance_1to1_real(reinterpret_cast<const float*>(input[idx].data()),
                                           reinterpret_cast<const float*>(output[idx].data()),
                                           length,
                                           nbatch,
                                           istride,
                                           idist,
                                           ostride,
                                           odist,
                                           linf_failures,
                                           linf_cutoff);
                    break;
                case rocfft_precision_double:
                    d = distance_1to1_real(reinterpret_cast<const double*>(input[idx].data()),
                                           reinterpret_cast<const double*>(output[idx].data()),
                                           length,
                                           nbatch,
                                           istride,
                                           idist,
                                           ostride,
                                           odist,
                                           linf_failures,
                                           linf_cutoff);
                    break;
                }
                dist.l_inf = std::max(d.l_inf, dist.l_inf);
                dist.l_2 += d.l_2 * d.l_2;
            }
            break;
        default:
            throw std::runtime_error("Invalid input and output types.");
            break;
        }
    }
    else if((itype == rocfft_array_type_complex_interleaved
             && otype == rocfft_array_type_complex_planar)
            || (itype == rocfft_array_type_hermitian_interleaved
                && otype == rocfft_array_type_hermitian_planar))
    {
        switch(precision)
        {
        case rocfft_precision_single:
            dist = distance_1to2(reinterpret_cast<const std::complex<float>*>(input[0].data()),
                                 reinterpret_cast<const float*>(output[0].data()),
                                 reinterpret_cast<const float*>(output[1].data()),
                                 length,
                                 nbatch,
                                 istride,
                                 idist,
                                 ostride,
                                 odist,
                                 linf_failures,
                                 linf_cutoff);
            break;
        case rocfft_precision_double:
            dist = distance_1to2(reinterpret_cast<const std::complex<double>*>(input[0].data()),
                                 reinterpret_cast<const double*>(output[0].data()),
                                 reinterpret_cast<const double*>(output[1].data()),
                                 length,
                                 nbatch,
                                 istride,
                                 idist,
                                 ostride,
                                 odist,
                                 linf_failures,
                                 linf_cutoff);
            break;
        }
        dist.l_2 *= dist.l_2;
    }
    else if((itype == rocfft_array_type_complex_planar
             && otype == rocfft_array_type_complex_interleaved)
            || (itype == rocfft_array_type_hermitian_planar
                && otype == rocfft_array_type_hermitian_interleaved))
    {
        switch(precision)
        {
        case rocfft_precision_single:
            dist = distance_1to2(reinterpret_cast<const std::complex<float>*>(output[0].data()),
                                 reinterpret_cast<const float*>(input[0].data()),
                                 reinterpret_cast<const float*>(input[1].data()),
                                 length,
                                 nbatch,
                                 ostride,
                                 odist,
                                 istride,
                                 idist,
                                 linf_failures,
                                 linf_cutoff);
            break;
        case rocfft_precision_double:
            dist = distance_1to2(reinterpret_cast<const std::complex<double>*>(output[0].data()),
                                 reinterpret_cast<const double*>(input[0].data()),
                                 reinterpret_cast<const double*>(input[1].data()),
                                 length,
                                 nbatch,
                                 ostride,
                                 odist,
                                 istride,
                                 idist,
                                 linf_failures,
                                 linf_cutoff);
            break;
        }
        dist.l_2 *= dist.l_2;
    }
    else
    {
        throw std::runtime_error("Invalid input and output types.");
    }
    dist.l_2 = sqrt(dist.l_2);
    return dist;
}

// Unroll arbitrary-dimension distance into specializations for 1-, 2-, 3-dimensions
template <typename Tallocator1,
          typename Tallocator2,
          typename Tint1,
          typename Tint2,
          typename Tint3>
inline VectorNorms distance(const std::vector<std::vector<char, Tallocator1>>& input,
                            const std::vector<std::vector<char, Tallocator2>>& output,
                            const std::vector<Tint1>&                          length,
                            const size_t                                       nbatch,
                            const rocfft_precision                             precision,
                            const rocfft_array_type                            itype,
                            const std::vector<Tint2>&                          istride,
                            const size_t                                       idist,
                            const rocfft_array_type                            otype,
                            const std::vector<Tint3>&                          ostride,
                            const size_t                                       odist,
                            std::vector<std::pair<size_t, size_t>>&            linf_failures,
                            const double                                       linf_cutoff)
{
    switch(length.size())
    {
    case 1:
        return distance(input,
                        output,
                        length[0],
                        nbatch,
                        precision,
                        itype,
                        istride[0],
                        idist,
                        otype,
                        ostride[0],
                        odist,
                        linf_failures,
                        linf_cutoff);
    case 2:
        return distance(input,
                        output,
                        std::make_tuple(length[0], length[1]),
                        nbatch,
                        precision,
                        itype,
                        std::make_tuple(istride[0], istride[1]),
                        idist,
                        otype,
                        std::make_tuple(ostride[0], ostride[1]),
                        odist,
                        linf_failures,
                        linf_cutoff);
    case 3:
        return distance(input,
                        output,
                        std::make_tuple(length[0], length[1], length[2]),
                        nbatch,
                        precision,
                        itype,
                        std::make_tuple(istride[0], istride[1], istride[2]),
                        idist,
                        otype,
                        std::make_tuple(ostride[0], ostride[1], ostride[2]),
                        odist,
                        linf_failures,
                        linf_cutoff);
    default:
        abort();
    }
}

// Compute the L-infinity and L-2 norm of abuffer with strides istride and
// length idist.  Data is std::complex.
template <typename Tcomplex, typename T1, typename T2>
inline VectorNorms norm_complex(const Tcomplex* input,
                                const T1&       whole_length,
                                const size_t    nbatch,
                                const T2&       istride,
                                const size_t    idist)
{
    double linf = 0.0;
    double l2   = 0.0;

    size_t idx_base   = 0;
    auto   partitions = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist)
    {
#pragma omp parallel for reduction(max : linf) reduction(+ : l2) num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            double     cur_linf = 0.0;
            double     cur_l2   = 0.0;
            auto       index    = partitions[part].first;
            const auto length   = partitions[part].second;
            do
            {
                const int idx = compute_index(index, istride, idx_base);

                const double rval = std::abs(input[idx].real());
                cur_linf          = std::max(rval, cur_linf);
                cur_l2 += rval * rval;

                const double ival = std::abs(input[idx].imag());
                cur_linf          = std::max(ival, cur_linf);
                cur_l2 += ival * ival;

            } while(increment_rowmajor(index, length));
            linf = std::max(linf, cur_linf);
            l2 += cur_l2;
        }
    }
    return {.l_2 = sqrt(l2), .l_inf = linf};
}

// Compute the L-infinity and L-2 norm of abuffer with strides istride and
// length idist.  Data is real-valued.
template <typename Tfloat, typename T1, typename T2>
inline VectorNorms norm_real(const Tfloat* input,
                             const T1&     whole_length,
                             const size_t  nbatch,
                             const T2&     istride,
                             const size_t  idist)
{
    double linf = 0.0;
    double l2   = 0.0;

    size_t idx_base   = 0;
    auto   partitions = partition_rowmajor(whole_length);
    for(size_t b = 0; b < nbatch; b++, idx_base += idist)
    {
#pragma omp parallel for reduction(max : linf) reduction(+ : l2) num_threads(partitions.size())
        for(size_t part = 0; part < partitions.size(); ++part)
        {
            double     cur_linf = 0.0;
            double     cur_l2   = 0.0;
            auto       index    = partitions[part].first;
            const auto length   = partitions[part].second;
            do
            {
                const int    idx = compute_index(index, istride, idx_base);
                const double val = std::abs(input[idx]);
                cur_linf         = std::max(val, cur_linf);
                cur_l2 += val * val;

            } while(increment_rowmajor(index, length));
            linf = std::max(linf, cur_linf);
            l2 += cur_l2;
        }
    }
    return {.l_2 = sqrt(l2), .l_inf = linf};
}

// Compute the L-infinity and L-2 norm of abuffer with strides istride and
// length idist.  Data format is given by precision and itype.
template <typename Tallocator1, typename T1, typename T2>
inline VectorNorms norm(const std::vector<std::vector<char, Tallocator1>>& input,
                        const T1&                                          length,
                        const size_t                                       nbatch,
                        const rocfft_precision                             precision,
                        const rocfft_array_type                            itype,
                        const T2&                                          istride,
                        const size_t                                       idist)
{
    VectorNorms norm;

    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_hermitian_interleaved:
        switch(precision)
        {
        case rocfft_precision_single:
            norm = norm_complex(reinterpret_cast<const std::complex<float>*>(input[0].data()),
                                length,
                                nbatch,
                                istride,
                                idist);
            break;
        case rocfft_precision_double:
            norm = norm_complex(reinterpret_cast<const std::complex<double>*>(input[0].data()),
                                length,
                                nbatch,
                                istride,
                                idist);
            break;
        }
        norm.l_2 *= norm.l_2;
        break;
    case rocfft_array_type_real:
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_planar:
        for(int idx = 0; idx < input.size(); ++idx)
        {
            VectorNorms n;
            switch(precision)
            {
            case rocfft_precision_single:
                n = norm_real(reinterpret_cast<const float*>(input[idx].data()),
                              length,
                              nbatch,
                              istride,
                              idist);
                break;
            case rocfft_precision_double:
                n = norm_real(reinterpret_cast<const double*>(input[idx].data()),
                              length,
                              nbatch,
                              istride,
                              idist);
                break;
            }
            norm.l_inf = std::max(n.l_inf, norm.l_inf);
            norm.l_2 += n.l_2 * n.l_2;
        }
        break;
    default:
        throw std::runtime_error("Invalid data type");
        break;
    }

    norm.l_2 = sqrt(norm.l_2);
    return norm;
}

// Unroll arbitrary-dimension norm into specializations for 1-, 2-, 3-dimensions
template <typename Tallocator1, typename T1, typename T2>
inline VectorNorms norm(const std::vector<std::vector<char, Tallocator1>>& input,
                        const std::vector<T1>&                             length,
                        const size_t                                       nbatch,
                        const rocfft_precision                             precision,
                        const rocfft_array_type                            itype,
                        const std::vector<T2>&                             istride,
                        const size_t                                       idist)
{
    switch(length.size())
    {
    case 1:
        return norm(input, length[0], nbatch, precision, itype, istride[0], idist);
    case 2:
        return norm(input,
                    std::make_tuple(length[0], length[1]),
                    nbatch,
                    precision,
                    itype,
                    std::make_tuple(istride[0], istride[1]),
                    idist);
    case 3:
        return norm(input,
                    std::make_tuple(length[0], length[1], length[2]),
                    nbatch,
                    precision,
                    itype,
                    std::make_tuple(istride[0], istride[1], istride[2]),
                    idist);
    default:
        abort();
    }
}

// Given a buffer of complex values stored in a vector of chars (or two vectors in the
// case of planar format), impose Hermitian symmetry.
// NB: length is the dimensions of the FFT, not the data layout dimensions.
template <typename Tfloat, typename Tallocator, typename Tsize>
inline void impose_hermitian_symmetry(std::vector<std::vector<char, Tallocator>>& vals,
                                      const std::vector<Tsize>&                   length,
                                      const std::vector<Tsize>&                   istride,
                                      const Tsize                                 idist,
                                      const Tsize                                 nbatch)
{
    switch(vals.size())
    {
    case 1:
    {
        // Complex interleaved data
        for(auto ibatch = 0; ibatch < nbatch; ++ibatch)
        {
            auto data = ((std::complex<Tfloat>*)vals[0].data()) + ibatch * idist;
            switch(length.size())
            {
            case 3:
                if(length[2] % 2 == 0)
                {
                    data[istride[2] * (length[2] / 2)].imag(0.0);
                }

                if(length[0] % 2 == 0 && length[2] % 2 == 0)
                {
                    data[istride[0] * (length[0] / 2) + istride[2] * (length[2] / 2)].imag(0.0);
                }
                if(length[1] % 2 == 0 && length[2] % 2 == 0)
                {
                    data[istride[1] * (length[1] / 2) + istride[2] * (length[2] / 2)].imag(0.0);
                }

                if(length[0] % 2 == 0 && length[1] % 2 == 0 && length[2] % 2 == 0)
                {
                    // clang format off
                    data[istride[0] * (length[0] / 2) + istride[1] * (length[1] / 2)
                         + istride[2] * (length[2] / 2)]
                        .imag(0.0);
                    // clang format off
                }

                // y-axis:
                for(auto j = 1; j < (length[1] + 1) / 2; ++j)
                {
                    data[istride[1] * (length[1] - j)] = std::conj(data[istride[1] * j]);
                }

                if(length[0] % 2 == 0)
                {
                    // y-axis at x-nyquist
                    for(auto j = 1; j < (length[1] + 1) / 2; ++j)
                    {
                        // clang format off
                        data[istride[0] * (length[0] / 2) + istride[1] * (length[1] - j)]
                            = std::conj(data[istride[0] * (length[0] / 2) + istride[1] * j]);
                        // clang format on
                    }
                }

                // x-axis:
                for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                {
                    data[istride[0] * (length[0] - i)] = std::conj(data[istride[0] * i]);
                }

                if(length[1] % 2 == 0)
                {
                    // x-axis at y-nyquist
                    for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                    {
                        // clang format off
                        data[istride[0] * (length[0] - i) + istride[1] * (length[1] / 2)]
                            = std::conj(data[istride[0] * i + istride[1] * (length[1] / 2)]);
                        // clang format on
                    }
                }

                // x-y plane:
                for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                {
                    for(auto j = 1; j < length[1]; ++j)
                    {
                        // clang format off
                        data[istride[0] * (length[0] - i) + istride[1] * (length[1] - j)]
                            = std::conj(data[istride[0] * i + istride[1] * j]);
                        // clang format on
                    }
                }

                if(length[2] % 2 == 0)
                {
                    // x-axis at z-nyquist
                    for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                    {
                        data[istride[0] * (length[0] - i) + istride[2] * (length[2] / 2)]
                            = std::conj(data[istride[0] * i + istride[2] * (length[2] / 2)]);
                    }
                    if(length[1] % 2 == 0)
                    {
                        // x-axis at yz-nyquist
                        for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                        {
                            data[istride[0] * (length[0] - i) + istride[2] * (length[2] / 2)]
                                = std::conj(data[istride[0] * i + istride[2] * (length[2] / 2)]);
                        }
                    }

                    // y-axis: at z-nyquist
                    for(auto j = 1; j < (length[1] + 1) / 2; ++j)
                    {
                        data[istride[1] * (length[1] - j) + istride[2] * (length[2] / 2)]
                            = std::conj(data[istride[1] * j + istride[2] * (length[2] / 2)]);
                    }

                    if(length[0] % 2 == 0)
                    {
                        // y-axis: at xz-nyquist
                        for(auto j = 1; j < (length[1] + 1) / 2; ++j)
                        {
                            // clang format off
                            data[istride[0] * (length[0] / 2) + istride[1] * (length[1] - j)
                                 + istride[2] * (length[2] / 2)]
                                = std::conj(data[istride[0] * (length[0] / 2) + istride[1] * j
                                                 + istride[2] * (length[2] / 2)]);
                            // clang format on
                        }
                    }

                    // x-y plane: at z-nyquist
                    for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                    {
                        for(auto j = 1; j < length[1]; ++j)
                        {
                            // clang format off
                            data[istride[0] * (length[0] - i) + istride[1] * (length[1] - j)
                                 + istride[2] * (length[2] / 2)]
                                = std::conj(data[istride[0] * i + istride[1] * j
                                                 + istride[2] * (length[2] / 2)]);
                            // clang format on
                        }
                    }
                }

                // fall-through
            case 2:
                if(length[1] % 2 == 0)
                {
                    data[istride[1] * (length[1] / 2)].imag(0.0);
                }

                if(length[0] % 2 == 0 && length[1] % 2 == 0)
                {
                    data[istride[0] * (length[0] / 2) + istride[1] * (length[1] / 2)].imag(0.0);
                }

                for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                {
                    data[istride[0] * (length[0] - i)] = std::conj(data[istride[0] * i]);
                }

                if(length[1] % 2 == 0)
                {
                    for(auto i = 1; i < (length[0] + 1) / 2; ++i)
                    {
                        data[istride[0] * (length[0] - i) + istride[1] * (length[1] / 2)]
                            = std::conj(data[istride[0] * i + istride[1] * (length[1] / 2)]);
                    }
                }

                // fall-through

            case 1:
                data[0].imag(0.0);

                if(length[0] % 2 == 0)
                {
                    data[istride[0] * (length[0] / 2)].imag(0.0);
                }
                break;

            default:
                throw std::runtime_error("Invalid dimension for imposeHermitianSymmetry");
                break;
            }
        }
        break;
    }
    case 2:
    {
        // Complex planar data
        for(auto ibatch = 0; ibatch < nbatch; ++ibatch)
        {
            auto rdata = ((Tfloat*)vals[0].data()) + ibatch * idist;
            auto idata = ((Tfloat*)vals[1].data()) + ibatch * idist;
            switch(length.size())
            {
            case 3:
                throw std::runtime_error("Not implemented");
                // FIXME: implement
            case 2:
                throw std::runtime_error("Not implemented");
                // FIXME: implement
            case 1:
                idata[0] = 0.0;
                if(length[0] % 2 == 0)
                {
                    idata[istride[0] * (length[0] / 2)] = 0.0;
                }
                break;
            default:
                throw std::runtime_error("Invalid dimension for imposeHermitianSymmetry");
                break;
            }
        }
        break;
    }
    default:
        throw std::runtime_error("Invalid data type");
        break;
    }
}

// Given an array type and transform length, strides, etc, load random floats in [0,1]
// into the input array of floats/doubles or complex floats/doubles, which is stored in a
// vector of chars (or two vectors in the case of planar format).
// lengths are the memory lengths (ie not the transform parameters)
template <typename Tfloat, typename Tallocator, typename Tint1>
inline void set_input(std::vector<std::vector<char, Tallocator>>& input,
                      const rocfft_array_type                     itype,
                      const Tint1&                                whole_length,
                      const Tint1&                                istride,
                      const size_t                                idist,
                      const size_t                                nbatch)
{
    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_hermitian_interleaved:
    {
        auto   idata      = (std::complex<Tfloat>*)input[0].data();
        size_t i_base     = 0;
        auto   partitions = partition_rowmajor(whole_length);
        for(auto b = 0; b < nbatch; b++, i_base += idist)
        {
#pragma omp parallel for num_threads(partitions.size())
            for(size_t part = 0; part < partitions.size(); ++part)
            {
                auto         index  = partitions[part].first;
                const auto   length = partitions[part].second;
                std::mt19937 gen(compute_index(index, istride, i_base));
                do
                {
                    const int                  i = compute_index(index, istride, i_base);
                    const std::complex<Tfloat> val((Tfloat)gen() / (Tfloat)gen.max(),
                                                   (Tfloat)gen() / (Tfloat)gen.max());
                    idata[i] = val;
                } while(increment_rowmajor(index, length));
            }
        }
        break;
    }
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_planar:
    {
        auto   ireal      = (Tfloat*)input[0].data();
        auto   iimag      = (Tfloat*)input[1].data();
        size_t i_base     = 0;
        auto   partitions = partition_rowmajor(whole_length);
        for(auto b = 0; b < nbatch; b++, i_base += idist)
        {
#pragma omp parallel for num_threads(partitions.size())
            for(size_t part = 0; part < partitions.size(); ++part)
            {
                auto         index  = partitions[part].first;
                const auto   length = partitions[part].second;
                std::mt19937 gen(compute_index(index, istride, i_base));
                do
                {
                    const int                  i = compute_index(index, istride, i_base);
                    const std::complex<Tfloat> val((Tfloat)gen() / (Tfloat)gen.max(),
                                                   (Tfloat)gen() / (Tfloat)gen.max());
                    ireal[i] = val.real();
                    iimag[i] = val.imag();
                } while(increment_rowmajor(index, length));
            }
        }
        break;
    }
    case rocfft_array_type_real:
    {
        auto   idata      = (Tfloat*)input[0].data();
        size_t i_base     = 0;
        auto   partitions = partition_rowmajor(whole_length);
        for(auto b = 0; b < nbatch; b++, i_base += idist)
        {
#pragma omp parallel for num_threads(partitions.size())
            for(size_t part = 0; part < partitions.size(); ++part)
            {
                auto         index  = partitions[part].first;
                const auto   length = partitions[part].second;
                std::mt19937 gen(compute_index(index, istride, i_base));
                do
                {
                    const int    i   = compute_index(index, istride, i_base);
                    const Tfloat val = (Tfloat)gen() / (Tfloat)gen.max();
                    idata[i]         = val;
                } while(increment_rowmajor(index, length));
            }
        }
        break;
    }
    default:
        throw std::runtime_error("Input layout format not yet supported");
        break;
    }
}

// unroll set_input for dimension 1, 2, 3
template <typename Tfloat, typename Tallocator>
inline void set_input(std::vector<std::vector<char, Tallocator>>& input,
                      const rocfft_array_type                     itype,
                      const std::vector<size_t>&                  length,
                      const std::vector<size_t>&                  istride,
                      const size_t                                idist,
                      const size_t                                nbatch)
{
    switch(length.size())
    {
    case 1:
        set_input<Tfloat>(input, itype, length[0], istride[0], idist, nbatch);
        break;
    case 2:
        set_input<Tfloat>(input,
                          itype,
                          std::make_tuple(length[0], length[1]),
                          std::make_tuple(istride[0], istride[1]),
                          idist,
                          nbatch);
        break;
    case 3:
        set_input<Tfloat>(input,
                          itype,
                          std::make_tuple(length[0], length[1], length[2]),
                          std::make_tuple(istride[0], istride[1], istride[2]),
                          idist,
                          nbatch);
        break;
    default:
        abort();
    }
}

// Compute the idist for a given transform based on the placeness, transform type, and
// data layout.
template <typename Tsize>
inline size_t set_idist(const rocfft_result_placement place,
                        const rocfft_transform_type   transformType,
                        const std::vector<Tsize>&     length,
                        const std::vector<Tsize>&     istride)
{
    const Tsize dim = length.size();

    // In-place 1D transforms need extra dist.
    if(transformType == rocfft_transform_type_real_forward && dim == 1
       && place == rocfft_placement_inplace)
    {
        return 2 * (length[0] / 2 + 1) * istride[0];
    }

    if(transformType == rocfft_transform_type_real_inverse && dim == 1)
    {
        return (length[0] / 2 + 1) * istride[0];
    }

    Tsize idist = (transformType == rocfft_transform_type_real_inverse)
                      ? (length[dim - 1] / 2 + 1) * istride[dim - 1]
                      : length[dim - 1] * istride[dim - 1];
    for(int i = 0; i < dim - 1; ++i)
    {
        idist = std::max(length[i] * istride[i], idist);
    }
    return idist;
}

// Compute the odist for a given transform based on the placeness, transform type, and
// data layout.  Row-major.
template <typename Tsize>
inline size_t set_odist(const rocfft_result_placement place,
                        const rocfft_transform_type   transformType,
                        const std::vector<Tsize>&     length,
                        const std::vector<Tsize>&     ostride)
{
    const Tsize dim = length.size();

    // In-place 1D transforms need extra dist.
    if(transformType == rocfft_transform_type_real_inverse && dim == 1
       && place == rocfft_placement_inplace)
    {
        return 2 * (length[0] / 2 + 1) * ostride[0];
    }

    if(transformType == rocfft_transform_type_real_forward && dim == 1)
    {
        return (length[0] / 2 + 1) * ostride[0];
    }

    Tsize odist = (transformType == rocfft_transform_type_real_forward)
                      ? (length[dim - 1] / 2 + 1) * ostride[dim - 1]
                      : length[dim - 1] * ostride[dim - 1];
    for(int i = 0; i < dim - 1; ++i)
    {
        odist = std::max(length[i] * ostride[i], odist);
    }
    return odist;
}

// Given a data type and precision, the distance between batches, and the batch size,
// allocate the required host buffer(s).
template <typename Allocator = std::allocator<char>, typename Tsize>
inline std::vector<std::vector<char, Allocator>> allocate_host_buffer(
    const rocfft_precision precision, const rocfft_array_type type, const Tsize size)
{
    const int nbuf
        = (type == rocfft_array_type_complex_planar || type == rocfft_array_type_hermitian_planar)
              ? 2
              : 1;
    std::vector<std::vector<char, Allocator>> buffers(nbuf);
    for(auto& i : buffers)
    {
        i.resize(size * var_size<Tsize>(precision, type));
    }
    return buffers;
}

// Given a data type and dimensions, fill the buffer, imposing Hermitian symmetry if
// necessary.
// NB: length is the logical size of the FFT, and not necessarily the data dimensions
template <typename Allocator = std::allocator<char>>
inline std::vector<std::vector<char, Allocator>> compute_input(const rocfft_params& params)
{
    auto input = allocate_host_buffer<Allocator>(params.precision, params.itype, params.isize);
    for(auto& i : input)
    {
        std::fill(i.begin(), i.end(), 0.0);
    }

    switch(params.precision)
    {
    case rocfft_precision_double:
        set_input<double>(
            input, params.itype, params.ilength(), params.istride, params.idist, params.nbatch);
        break;
    case rocfft_precision_single:
        set_input<float>(
            input, params.itype, params.ilength(), params.istride, params.idist, params.nbatch);
        break;
    }

    if(params.itype == rocfft_array_type_hermitian_interleaved
       || params.itype == rocfft_array_type_hermitian_planar)
    {
        switch(params.precision)
        {
        case rocfft_precision_double:
            impose_hermitian_symmetry<double>(
                input, params.length, params.istride, params.idist, params.nbatch);
            break;
        case rocfft_precision_single:
            impose_hermitian_symmetry<float>(
                input, params.length, params.istride, params.idist, params.nbatch);
            break;
        }
    }
    return input;
}

// Check that the input and output types are consistent.
inline void check_iotypes(const rocfft_result_placement place,
                          const rocfft_transform_type   transformType,
                          const rocfft_array_type       itype,
                          const rocfft_array_type       otype)
{
    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_interleaved:
    case rocfft_array_type_hermitian_planar:
    case rocfft_array_type_real:
        break;
    default:
        throw std::runtime_error("Invalid Input array type format");
    }

    switch(otype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_interleaved:
    case rocfft_array_type_hermitian_planar:
    case rocfft_array_type_real:
        break;
    default:
        throw std::runtime_error("Invalid Input array type format");
    }

    // Check that format choices are supported
    if(transformType != rocfft_transform_type_real_forward
       && transformType != rocfft_transform_type_real_inverse)
    {
        if(place == rocfft_placement_inplace && itype != otype)
        {
            throw std::runtime_error(
                "In-place transforms must have identical input and output types");
        }
    }

    bool okformat = true;
    switch(itype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_complex_planar:
        okformat = (otype == rocfft_array_type_complex_interleaved
                    || otype == rocfft_array_type_complex_planar);
        break;
    case rocfft_array_type_hermitian_interleaved:
    case rocfft_array_type_hermitian_planar:
        okformat = otype == rocfft_array_type_real;
        break;
    case rocfft_array_type_real:
        okformat = (otype == rocfft_array_type_hermitian_interleaved
                    || otype == rocfft_array_type_hermitian_planar);
        break;
    default:
        throw std::runtime_error("Invalid Input array type format");
    }
    switch(otype)
    {
    case rocfft_array_type_complex_interleaved:
    case rocfft_array_type_complex_planar:
    case rocfft_array_type_hermitian_interleaved:
    case rocfft_array_type_hermitian_planar:
    case rocfft_array_type_real:
        break;
    default:
        okformat = false;
    }
    if(!okformat)
    {
        throw std::runtime_error("Invalid combination of Input/Output array type formats");
    }
}

// Check that the input and output types are consistent.  If they are unset, assign
// default values based on the transform type.
inline void check_set_iotypes(const rocfft_result_placement place,
                              const rocfft_transform_type   transformType,
                              rocfft_array_type&            itype,
                              rocfft_array_type&            otype)
{
    if(itype == rocfft_array_type_unset)
    {
        switch(transformType)
        {
        case rocfft_transform_type_complex_forward:
        case rocfft_transform_type_complex_inverse:
            itype = rocfft_array_type_complex_interleaved;
            break;
        case rocfft_transform_type_real_forward:
            itype = rocfft_array_type_real;
            break;
        case rocfft_transform_type_real_inverse:
            itype = rocfft_array_type_hermitian_interleaved;
            break;
        default:
            throw std::runtime_error("Invalid transform type");
        }
    }
    if(otype == rocfft_array_type_unset)
    {
        switch(transformType)
        {
        case rocfft_transform_type_complex_forward:
        case rocfft_transform_type_complex_inverse:
            otype = rocfft_array_type_complex_interleaved;
            break;
        case rocfft_transform_type_real_forward:
            otype = rocfft_array_type_hermitian_interleaved;
            break;
        case rocfft_transform_type_real_inverse:
            otype = rocfft_array_type_real;
            break;
        default:
            throw std::runtime_error("Invalid transform type");
        }
    }

    check_iotypes(place, transformType, itype, otype);
}

#endif
