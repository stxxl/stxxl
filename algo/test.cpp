#include <iostream>



struct no_key_tag {};
struct key_tag : public no_key_tag { };

struct structure_has_key
{};


struct with_key : public structure_has_key
{
    typedef int key_type;

    key_type _key;
    int data;
    key_type key()
    {
        return _key;
    };
};

struct without_key
{
    int data;
};



template <typename _Tp>
struct container_iterator
{
    typedef _Tp value_type;
};


template<typename _Tp>
bool has_key()
{
    return false;
}


template<>
bool has_key< structure_has_key * >()
{
    return true;
}

template<>
bool has_key< container_iterator< structure_has_key> >()
{
    return true;
}


template<typename _Iter>
struct iterator_traits
{
    //  typedef _Iter::value_type value_type;
};

/*
   template<typename _Tp>
   struct iterator_traits<_Tp>
   {
   typedef _Tp value_type;
   };
 */



template <typename _Iter>
void _some_algorithm_with_use_of_keys(_Iter /*first*/, _Iter /*last*/)
{
    std::cout << "Use keys" << std::endl;
}

template <typename _Iter>
void _some_algorithm_without_use_of_keys(_Iter /*first*/, _Iter /*last*/)
{
    std::cout << "Don't use keys" << std::endl;
}

template <typename _Iter>
void some_algorithm(_Iter first, _Iter last)
{
    //  some_algorithm(first,last, key_present(first) );
    if (has_key < _Iter > ())
        _some_algorithm_with_use_of_keys(first, last);

    else
        _some_algorithm_without_use_of_keys(first, last);

}

int main()
{
    with_key * f = NULL, * l = NULL;

    some_algorithm(f, l);



    return 0;
}
