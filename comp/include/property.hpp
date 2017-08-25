#ifndef __PROPERTY_HPP__
#define __PROPERTY_HPP__


namespace diversity
{
    template <typename T, typename F>
    struct Property
    {
    friend F;

    public:
        Property(T val);

        Property(Property&) = delete;

        Property(Property&&) = delete;

        Property& operator = (const T&) = delete;

        Property& operator = (const Property&) = delete;

        T& operator = (const Property&) const = delete;

        bool operator == (const T &op) const;

        operator T& () const;

        operator Property& () const = delete;

        T operator -> () const;

    protected:
        void operator () (const T &val) const;

    private:
        mutable T val;
    }; /* Property */
} /* diversity */

/* inline methods ----------------------------------------------------------- */

template <typename T, typename F>
inline diversity::Property<T, F>::Property(T val)
    : val { val }
{

} /* Property */

#if 0
template <typename T, typename F>
inline T& diversity::Property<T, F>::operator = (const Property&) const
{
    return this->val;
} /* operator = */
#endif

template <typename T, typename F>
inline bool diversity::Property<T, F>::operator == (const T &op) const
{
    return this->val == op;
} /* operator == */

template <typename T, typename F>
inline diversity::Property<T, F>::operator T& () const
{
    return this->val;
} /* operator T& */

template <typename T, typename F>
inline T diversity::Property<T, F>::operator -> () const
{
    return this->val;
} /* operator T& */

template <typename T, typename F>
inline void diversity::Property<T, F>::operator () (const T &val) const
{
    this->val = val;
} /* Set */


#endif /* __PROPERTY_HPP__ */
