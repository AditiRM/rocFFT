// Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include <optional>

#include "function_pool.h"
#include "kernel_launch.h"
#include "rtc_stockham_gen.h"
#include "rtc_stockham_kernel.h"
#include "tree_node.h"

#include "device/kernel-generator-embed.h"

RTCKernel::RTCGenerator RTCKernelStockham::generate_from_node(const TreeNode&    node,
                                                              const std::string& gpu_arch,
                                                              bool               enable_callbacks)
{
    RTCGenerator   generator;
    function_pool& pool = function_pool::get_function_pool();

    std::optional<StockhamGeneratorSpecs> specs;
    std::optional<StockhamGeneratorSpecs> specs2d;

    // if scale factor is enabled, we force RTC for this kernel
    bool enable_scaling = node.IsScalingEnabled();

    SBRC_TRANSPOSE_TYPE transpose_type = NONE;

    // SBRC variants look in the function pool for plain BLOCK_RC to
    // learn the block width, then decide on the transpose type once
    // that's known.
    auto         pool_scheme = node.scheme;
    unsigned int static_dim  = node.length.size();
    if(pool_scheme == CS_KERNEL_STOCKHAM_TRANSPOSE_XY_Z
       || pool_scheme == CS_KERNEL_STOCKHAM_TRANSPOSE_Z_XY
       || pool_scheme == CS_KERNEL_STOCKHAM_R_TO_CMPLX_TRANSPOSE_Z_XY)
    {
        pool_scheme = CS_KERNEL_STOCKHAM_BLOCK_RC;
        // These are all 3D kernels, but are sometimes shoehorned
        // into 2D plans.  Make sure they get at least 3 dims.
        if(static_dim == 2)
            static_dim = 3;
    }

    std::optional<FFTKernel> kernel;

    // find function pool entry so we can construct specs for the generator
    FMKey key;
    switch(pool_scheme)
    {
    case CS_KERNEL_STOCKHAM:
    case CS_KERNEL_STOCKHAM_BLOCK_CC:
    case CS_KERNEL_STOCKHAM_BLOCK_CR:
    case CS_KERNEL_STOCKHAM_BLOCK_RC:
    {
        // these go into the function pool normally and are passed to
        // the generator as-is
        key    = fpkey(node.length[0], node.precision, pool_scheme);
        kernel = pool.get_kernel(key);
        // already precompiled?
        if(kernel->device_function && !enable_scaling)
        {
            return generator;
        }

        // for SBRC variants, get the "real" kernel using the block
        // width and correct transpose type
        if(pool_scheme == CS_KERNEL_STOCKHAM_BLOCK_RC)
        {
            transpose_type = node.sbrc_transpose_type(kernel->transforms_per_block);
            key            = fpkey(node.length[0], node.precision, node.scheme, transpose_type);
            kernel         = pool.get_kernel(key);
        }

        std::vector<unsigned int> factors;
        std::copy(kernel->factors.begin(), kernel->factors.end(), std::back_inserter(factors));
        std::vector<unsigned int> precisions = {static_cast<unsigned int>(node.precision)};

        specs.emplace(factors,
                      std::vector<unsigned int>(),
                      precisions,
                      static_cast<unsigned int>(kernel->workgroup_size),
                      PrintScheme(node.scheme));
        specs->threads_per_transform = kernel->threads_per_transform[0];
        specs->half_lds              = kernel->half_lds;
        specs->direct_to_from_reg    = kernel->direct_to_from_reg;
        break;
    }
    case CS_KERNEL_2D_SINGLE:
    {
        key    = fpkey(node.length[0], node.length[1], node.precision, node.scheme);
        kernel = pool.get_kernel(key);
        // already precompiled?
        if(kernel->device_function && !enable_scaling)
        {
            return generator;
        }

        std::vector<unsigned int> factors1d;
        std::vector<unsigned int> factors2d;
        std::vector<unsigned int> precisions = {static_cast<unsigned int>(node.precision)};

        // need to break down factors into first dim and second dim
        size_t len0_remain = node.length[0];
        for(auto& f : kernel->factors)
        {
            len0_remain /= f;
            if(len0_remain > 0)
            {
                factors1d.push_back(f);
            }
            else
            {
                factors2d.push_back(f);
            }
        }

        specs.emplace(factors1d,
                      factors2d,
                      precisions,
                      static_cast<unsigned int>(kernel->workgroup_size),
                      PrintScheme(node.scheme));
        specs->threads_per_transform = kernel->threads_per_transform[0];
        specs->half_lds              = kernel->half_lds;

        specs2d.emplace(factors2d,
                        factors1d,
                        precisions,
                        static_cast<unsigned int>(kernel->workgroup_size),
                        PrintScheme(node.scheme));
        specs2d->threads_per_transform = kernel->threads_per_transform[1];
        specs2d->half_lds              = kernel->half_lds;
        break;
    }
    default:
    {
        return generator;
    }
    }

    // static_dim has what the plan requires, and RTC kernels are
    // built for exactly that dimension.  But kernels that are
    // compiled ahead of time are general across all dims and take
    // 'dim' as an argument.  So set static_dim to 0 to communicate
    // this to the generator and launch machinery.
    if(kernel && kernel->aot_rtc)
        static_dim = 0;
    specs->static_dim = static_dim;

    bool unit_stride = node.inStride.front() == 1 && node.outStride.front() == 1;

    generator.generate_name = [=, &node]() {
        return stockham_rtc_kernel_name(node.scheme,
                                        node.length[0],
                                        node.scheme == CS_KERNEL_2D_SINGLE ? node.length[1] : 0,
                                        static_dim,
                                        node.direction,
                                        node.precision,
                                        node.placement,
                                        node.inArrayType,
                                        node.outArrayType,
                                        unit_stride,
                                        node.largeTwdBase,
                                        node.ltwdSteps,
                                        node.ebtype,
                                        node.dir2regMode,
                                        node.intrinsicMode,
                                        transpose_type,
                                        enable_callbacks,
                                        node.IsScalingEnabled());
    };

    generator.generate_src = [=, &node](const std::string& kernel_name) {
        return stockham_rtc(*specs,
                            specs2d ? *specs2d : *specs,
                            nullptr,
                            kernel_name,
                            node.scheme,
                            node.direction,
                            node.precision,
                            node.placement,
                            node.inArrayType,
                            node.outArrayType,
                            unit_stride,
                            node.largeTwdBase,
                            node.ltwdSteps,
                            node.ebtype,
                            node.dir2regMode,
                            node.intrinsicMode,
                            transpose_type,
                            enable_callbacks,
                            node.IsScalingEnabled());
    };

    generator.construct_rtckernel
        = [](const std::string& kernel_name, const std::vector<char>& code) {
              return std::unique_ptr<RTCKernel>(new RTCKernelStockham(kernel_name, code));
          };
    return generator;
}

RTCKernelArgs RTCKernelStockham::get_launch_args(DeviceCallIn& data)
{
    // construct arguments to pass to the kernel
    RTCKernelArgs kargs;

    // twiddles
    kargs.append_ptr(data.node->twiddles);
    // large 1D twiddles
    if(data.node->scheme == CS_KERNEL_STOCKHAM_BLOCK_CC)
        kargs.append_ptr(data.node->twiddles_large);
    if(!hardcoded_dim)
        kargs.append_size_t(data.node->length.size());
    // lengths
    kargs.append_ptr(kargs_lengths(data.node->devKernArg));
    // stride in/out
    kargs.append_ptr(kargs_stride_in(data.node->devKernArg));
    if(data.node->placement == rocfft_placement_notinplace)
        kargs.append_ptr(kargs_stride_out(data.node->devKernArg));
    // nbatch
    kargs.append_size_t(data.node->batch);
    // lds padding
    kargs.append_unsigned_int(data.node->lds_padding);
    // callback params
    kargs.append_ptr(data.callbacks.load_cb_fn);
    kargs.append_ptr(data.callbacks.load_cb_data);
    kargs.append_unsigned_int(data.callbacks.load_cb_lds_bytes);
    kargs.append_ptr(data.callbacks.store_cb_fn);
    kargs.append_ptr(data.callbacks.store_cb_data);

    // buffer pointers
    kargs.append_ptr(data.bufIn[0]);
    if(array_type_is_planar(data.node->inArrayType))
        kargs.append_ptr(data.bufIn[1]);
    if(data.node->placement == rocfft_placement_notinplace)
    {
        kargs.append_ptr(data.bufOut[0]);
        if(array_type_is_planar(data.node->outArrayType))
            kargs.append_ptr(data.bufOut[1]);
    }
    return kargs;
}
