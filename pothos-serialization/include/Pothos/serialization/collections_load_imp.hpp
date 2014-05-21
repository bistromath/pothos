#ifndef  POTHOS_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP
#define POTHOS_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1020)
#  pragma warning (disable : 4786) // too long name, harmless warning
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// collections_load_imp.hpp: serialization for loading stl collections

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// helper function templates for serialization of collections

#include <Pothos/serialization/impl/assert.hpp>
#include <cstddef> // size_t
#include <Pothos/serialization/impl/config.hpp> // msvc 6.0 needs this for warning suppression
#if defined(POTHOS_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t; 
} // namespace std
#endif
#include <Pothos/serialization/impl/detail/workaround.hpp>

#include <Pothos/archive/detail/basic_iarchive.hpp>
#include <Pothos/serialization/access.hpp>
#include <Pothos/serialization/nvp.hpp>
#include <Pothos/serialization/detail/stack_constructor.hpp>
#include <Pothos/serialization/collection_size_type.hpp>
#include <Pothos/serialization/item_version_type.hpp>

namespace Pothos{
namespace serialization {
namespace stl {

//////////////////////////////////////////////////////////////////////
// implementation of serialization for STL containers
//

// sequential container input
template<class Archive, class Container>
struct archive_input_seq
{
    inline POTHOS_DEDUCED_TYPENAME Container::iterator
    operator()(
        Archive &ar, 
        Container &s, 
        const unsigned int v,
        POTHOS_DEDUCED_TYPENAME Container::iterator hint
    ){
        typedef POTHOS_DEDUCED_TYPENAME Container::value_type type;
        detail::stack_construct<Archive, type> t(ar, v);
        // borland fails silently w/o full namespace
        ar >> Pothos::serialization::make_nvp("item", t.reference());
        s.push_back(t.reference());
        ar.reset_object_address(& s.back() , & t.reference());
        return hint;
    }
};

// map input
template<class Archive, class Container>
struct archive_input_map
{
    inline POTHOS_DEDUCED_TYPENAME Container::iterator
    operator()(
        Archive &ar, 
        Container &s, 
        const unsigned int v,
        POTHOS_DEDUCED_TYPENAME Container::iterator hint
    ){
        typedef POTHOS_DEDUCED_TYPENAME Container::value_type type;
        detail::stack_construct<Archive, type> t(ar, v);
        // borland fails silently w/o full namespace
        ar >> Pothos::serialization::make_nvp("item", t.reference());
        POTHOS_DEDUCED_TYPENAME Container::iterator result = 
            s.insert(hint, t.reference());
        // note: the following presumes that the map::value_type was NOT tracked
        // in the archive.  This is the usual case, but here there is no way
        // to determine that.  
        ar.reset_object_address(
            & (result->second),
            & t.reference().second
        );
        return result;
    }
};

// set input
template<class Archive, class Container>
struct archive_input_set
{
    inline POTHOS_DEDUCED_TYPENAME Container::iterator
    operator()(
        Archive &ar, 
        Container &s, 
        const unsigned int v,
        POTHOS_DEDUCED_TYPENAME Container::iterator hint
    ){
        typedef POTHOS_DEDUCED_TYPENAME Container::value_type type;
        detail::stack_construct<Archive, type> t(ar, v);
        // borland fails silently w/o full namespace
        ar >> Pothos::serialization::make_nvp("item", t.reference());
        POTHOS_DEDUCED_TYPENAME Container::iterator result = 
            s.insert(hint, t.reference());
        ar.reset_object_address(& (* result), & t.reference());
        return result;
    }
};

template<class Container>
class reserve_imp
{
public:
    void operator()(Container &s, std::size_t count) const {
        s.reserve(count);
    }
};

template<class Container>
class no_reserve_imp
{
public:
    void operator()(Container & /* s */, std::size_t /* count */) const{}
};

template<class Archive, class Container, class InputFunction, class R>
inline void load_collection(Archive & ar, Container &s)
{
    s.clear();
    collection_size_type count;
    const Pothos::archive::library_version_type library_version(
        ar.get_library_version()
    );
    // retrieve number of elements
    item_version_type item_version(0);
    ar >> POTHOS_SERIALIZATION_NVP(count);
    if(Pothos::archive::library_version_type(3) < library_version){
        ar >> POTHOS_SERIALIZATION_NVP(item_version);
    }

    R rx;
    rx(s, count);
    InputFunction ifunc;
    POTHOS_DEDUCED_TYPENAME Container::iterator hint;
    hint = s.begin();
    while(count-- > 0){
        hint = ifunc(ar, s, item_version, hint);
    }
}

} // namespace stl 
} // namespace serialization
} // namespace boost

#endif //BOOST_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP
