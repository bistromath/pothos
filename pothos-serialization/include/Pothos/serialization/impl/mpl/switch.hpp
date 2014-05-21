
#ifndef POTHOS_MPL_SWITCH_HPP_INCLUDED
#define POTHOS_MPL_SWITCH_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: switch.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <Pothos/serialization/impl/mpl/find_if.hpp>
#include <Pothos/serialization/impl/mpl/deref.hpp>
#include <Pothos/serialization/impl/mpl/lambda.hpp>
#include <Pothos/serialization/impl/mpl/apply.hpp>
#include <Pothos/serialization/impl/mpl/pair.hpp>
#include <Pothos/serialization/impl/mpl/aux_/na_spec.hpp>
#include <Pothos/serialization/impl/mpl/aux_/lambda_support.hpp>

namespace Pothos { namespace mpl {

template< 
      typename POTHOS_MPL_AUX_NA_PARAM(Body)
    , typename POTHOS_MPL_AUX_NA_PARAM(T)
    >
struct switch_
{
    typedef typename find_if<
          Body
        , apply1< lambda< first<_1> >, T >
        >::type iter_;
        
    typedef typename deref<iter_>::type pair_;
    typedef typename lambda< typename second<pair_>::type >::type f_;
    typedef typename apply1<f_,T>::type type;

    POTHOS_MPL_AUX_LAMBDA_SUPPORT(2,switch_,(Body,T))
};

POTHOS_MPL_AUX_NA_SPEC(2, switch_)

}}

#endif // BOOST_MPL_SWITCH_HPP_INCLUDED
